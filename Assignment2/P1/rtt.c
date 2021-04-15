#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <netinet/ip.h>
#include <signal.h>
#include <sys/wait.h>
#define BUFSIZE 1500
#define PERHOSTLIM 3

typedef struct myRTT4{
  char addr[40];//IP addr
  int ipv;//IP version
  double rtt[PERHOSTLIM];
  int sockfd;
  struct sockaddr_in dest;
} myRTT4;

typedef struct myRTT6{
  char addr[40];//IP addr
  int ipv;//IP version
  double rtt[PERHOSTLIM];
  int sockfd;
  struct sockaddr_in6 dest;
} myRTT6;

void setV4Dest(myRTT4 *);
void setV6Dest(myRTT6 *);
int getIPVersion(const char *);
int getNextIP(FILE *,char * ,int *);
int getSocket(int);
unsigned short cal_chksum(unsigned short *, int);
void tv_sub(struct timeval *out, struct timeval *in);
void send_packetV4(int seq,myRTT4*);
void send_packetV6(int seq,myRTT6*);
void recv_packetV4(myRTT4 * r);
int procV4(myRTT4 *,char *, int ,struct timeval *);
void recv_packetV6(myRTT6 * r);
int procV6(myRTT6 *,char *, int ,struct timeval *);

void die(const char * msg,int type){
  if(type==0){
    perror(msg);
    exit(1);
  }else{
    puts(msg);
  }
}
int datalen = 56;
int main(int argc, char *argv[])
{
    int totalPinged = 0;
    int totalv4 = 0;
    int totalv6 = 0;
    if(argc<2||argc>2){
      puts("Invalid usage: ./a.out <IP filename name>");
      exit(1);
    }
    FILE * fp = fopen(argv[1],"r");
    if(fp==NULL){
        die("The input file does not exist",0);
    }
    puts("***** rtt program started *****");
    time_t start = time(NULL);
    int done = 0;
    while(!done){
          char addr[128];
          memset(addr,'\0',128);
          int ret = getNextIP(fp,addr,&done);
          if(!ret){
            continue;
          }
          int type = getIPVersion(addr);
          if(type<0){
            continue;
          }
          totalPinged++;
          if(type == 4){
              totalv4++;
              myRTT4 r;
              memset(r.addr,'\0',40);
              strcpy(r.addr,addr);
              r.ipv = type;
              r.sockfd = getSocket(type);
              setV4Dest(&r);
              for(int i=0;i<PERHOSTLIM;i++){
                  r.rtt[i]=-1;
              }
              for(int i=0;i<PERHOSTLIM;i++){
                  //send and recv
                  send_packetV4(i,&r);
                  recv_packetV4(&r);
              }
              printf("HOST: %s\t",r.addr);
              for(int i=0;i<PERHOSTLIM;i++){
                  printf("RTT(%d) = %.3f ms ",i+1 ,r.rtt[i]);
              }
              puts("");
              close(r.sockfd);
          }else if(type ==6){
              totalv6++;
              myRTT6 r;
              memset(r.addr,'\0',40);
              strcpy(r.addr,addr);
              r.ipv = type;
              r.sockfd = getSocket(type);
              setV6Dest(&r);
              for(int i=0;i<PERHOSTLIM;i++){
                  r.rtt[i]=-1;
              }
              for(int i=0;i<PERHOSTLIM;i++){
                  send_packetV6(i,&r);
                  recv_packetV6(&r);
              }
              close(r.sockfd);
          }
   }
   time_t end=time(NULL);
   fclose(fp);
   puts("***** rtt program ended *****");
   printf("Total %d (IPv4: %d + Ipv6: %d) IP addresses PINGED\n",totalPinged,totalv4,totalv6);
   time_t gap =end-start;
   int hrs = (gap/3600);
   gap%=3600;
   int mins = gap/60;
   gap%=60;
   int sec = gap;
   printf("Total time taken: %d hrs:%d mins: %d secs\n",hrs,mins,sec);
   return 0;
}

int getIPVersion(const char *addr) {
    char buf[16];
    if (inet_pton(AF_INET, addr, buf)) {
        return 4;
    } else if (inet_pton(AF_INET6, addr, buf)) {
        return 6;
    }
    return -1;
}
int getNextIP(FILE * fp,char * msg,int * status){
  if(feof(fp)){
    *status = 1;
    return 0;
  }
  fscanf(fp,"%[^\n]",msg);
  fgetc(fp);
  if(strlen(msg)==0||feof(fp)){
    *status = 1;
    return 0;
  }
  *status = 0;
  return 1;
}
void setV4Dest(myRTT4 * r){
      (r->dest).sin_family = AF_INET;
      (r->dest).sin_addr.s_addr = inet_addr(r->addr);
      printf("PING FOR %s\n",inet_ntoa((r->dest).sin_addr));
}
void setV6Dest(myRTT6 * r){
    (r->dest).sin6_family = AF_INET6;
    inet_pton(AF_INET6,r->addr,(r->dest).sin6_addr.s6_addr);
    printf("PING FOR %s\n",r->addr);
    if(IN6_IS_ADDR_V4MAPPED(&((r->dest).sin6_addr))){
      printf("Can't PING IPv4 mapped IPv6 address\n");
    }
}
int getSocket(int ipv){
  if(ipv == 4){
    //IPv4
    int sock = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
    if(sock <0 ){
      die("Socket failed",0);
    }
    int size = 60*1024;
    setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size));
    struct timeval tv ={.tv_sec=1};
    setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
    setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
    return sock;
  }else if(ipv == 6){
    //IPv6
    int sock = socket(AF_INET6,SOCK_RAW,IPPROTO_ICMPV6);
    if(sock <0 ){
      die("Socket failed",0);
    }
    struct icmp6_filter filt;
    ICMP6_FILTER_SETBLOCKALL(&filt);
    ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY,&filt);
    ICMP6_FILTER_SETPASS(ICMP6_ECHO_REQUEST,&filt);
    setsockopt(sock,IPPROTO_IPV6,ICMP6_FILTER,&filt,sizeof(filt));
    int size = 60*1024;
    setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size));
    struct timeval tv ={.tv_sec=1};
    setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
    setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
    return sock;
  }
  return -1;
}

unsigned short cal_chksum(unsigned short *addr, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;
    while (nleft > 1)
    {
        sum +=  *w++;
        nleft -= 2;
    }
    if (nleft == 1)
    {
        *(unsigned char*)(&answer) = *(unsigned char*)w;
        sum += answer;
    }
    sum = (sum >> 16) + (sum &0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

void send_packetV4(int seq,myRTT4 * r)
{
  int sockfd = r->sockfd;
   int i, packsize;
   char sendpacket [BUFSIZE];
   struct icmp *icmp;
   struct timeval *tval;
   icmp = (struct icmp*)sendpacket;
   icmp->icmp_type = ICMP_ECHO;
   icmp->icmp_code = 0;
   icmp->icmp_cksum = 0;
   icmp->icmp_seq = seq;
   icmp->icmp_id = getpid();
   packsize = 8+datalen;
   memset(icmp->icmp_data,0xa5,datalen);
   tval = (struct timeval*)icmp->icmp_data;
   gettimeofday(tval, NULL);
   icmp->icmp_cksum = cal_chksum((unsigned short*)icmp, packsize);
   if (sendto(sockfd,sendpacket, packsize, 0,(struct sockaddr *)&(r->dest), sizeof((r->dest))) < 0)
   {
       if(errno==EWOULDBLOCK){
           die("timeout",1);
           return;
       }
        die("sendto error",1);
   }
}
void recv_packetV4(myRTT4 * r)
{
    struct sockaddr_in cli;
    char recvpacket [BUFSIZE];
    int n, clilen;
    clilen = sizeof(cli);
    if ((n = recvfrom(r->sockfd, recvpacket, sizeof(recvpacket), 0, (struct sockaddr*) &cli, &clilen)) < 0)
        {
            if(errno==EWOULDBLOCK){
                die("timeout",1);
                return;
            }
           die("recvfrom error",1);
           return;
        }
    struct timeval tvrecv;
    gettimeofday(&tvrecv, NULL);
    if (procV4(r,recvpacket, n,&tvrecv) < 0){
        die("Failed to process packet",1);
    }
}
int procV4(myRTT4 * r,char *recvbuf, int len,struct timeval * tvrecv)
{
    double rtt;
    int hlen1, icmplen;
    struct ip *ip;
    struct icmp *icmp;
    struct timeval *tvsend;
    ip = (struct ip*)recvbuf;
    hlen1 = ip->ip_hl << 2;
    if(ip->ip_p!=IPPROTO_ICMP){
      return 0;
    }
    icmp = (struct icmp*)(recvbuf + hlen1);
    icmplen = len - hlen1;
    if (icmplen< 8)
    {
        printf("ICMP packets\'s length is less than 8\n");
        return  -1;
    }
    if ((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == getpid()))
    {
        if(icmplen < 16){
          printf("ICMP packets\'s length is less than 16, Not enough to be processed\n");
          return -1;
        }
        tvsend = (struct timeval*)icmp->icmp_data;
        tv_sub(tvrecv, tvsend);
        rtt = tvrecv->tv_sec * 1000+tvrecv->tv_usec / 1000;
        r->rtt[icmp->icmp_seq] = rtt;
        return 1;
    }
    else
        return  - 1;
}
void tv_sub(struct timeval *out, struct timeval *in)
{
    if ((out->tv_usec -= in->tv_usec) < 0)
    {
        --out->tv_sec;
        out->tv_usec += 1000000;
    }
     out->tv_sec -= in->tv_sec;
}
void send_packetV6(int seq,myRTT6 * r)
{
   int sockfd = r->sockfd;
   int i, packsize;
   char sendpacket [BUFSIZE];
   struct icmp6_hdr *icmp6;
   struct timeval *tval;
   icmp6 = (struct icmp6_hdr*)sendpacket;
   icmp6->icmp6_type = ICMP6_ECHO_REQUEST;
   icmp6->icmp6_code = 0;
   icmp6->icmp6_seq = seq;
   icmp6->icmp6_id = getpid();
   //checksum handled by kernel for us
   memset((icmp6+1),0xa5,datalen);
   tval = (struct timeval*)(icmp6+1);
   gettimeofday(tval, NULL);
   packsize = 8+datalen;
   if (sendto(sockfd,sendpacket, packsize, 0,(struct sockaddr *)&(r->dest), sizeof((r->dest))) < 0)
   {
       if(errno==EWOULDBLOCK){
           die("timeout",1);
           return;
       }
        perror("sendto error");
   }
}
void recv_packetV6(myRTT6 * r)
{
    struct sockaddr_in6 cli;
    int n, clilen;
    clilen = sizeof(cli);
    char recvpacket [BUFSIZE];
    if ((n = recvfrom(r->sockfd,recvpacket,sizeof(recvpacket),0,(struct sockaddr*)&cli,&clilen)) < 0)
        {
            if(errno==EWOULDBLOCK){
                die("timeout",1);
                return;
            }
            perror("recvfrom error");
           return;
        }
    struct timeval tvrecv;
    gettimeofday(&tvrecv, NULL);
    if (procV6(r,recvpacket, n,&tvrecv) < 0){
        die("Failed to process packet",1);
    }
}
int procV6(myRTT6 * r,char *recvbuf, int len,struct timeval * tvrecv)
{
    double rtt;
    struct icmp6_hdr *icmp6;
    struct timeval *tvsend;
    icmp6 = (struct icmp6_hdr*)recvbuf;
    if(len<8){
      printf("ICMP packets\'s length is less than 8\n");
      return  -1;
    }
    if(icmp6->icmp6_type == ICMP6_ECHO_REPLY && icmp6->icmp6_id == getpid()){
      if(len < 16){
        printf("ICMP packets\'s length is less than 16, Not enough to be processed\n");
        return -1;
      }
      tvsend = (struct timeval*)(icmp6+1);
      tv_sub(tvrecv, tvsend);
      rtt = tvrecv->tv_sec * 1000+tvrecv->tv_usec / 1000;
      r->rtt[icmp6->icmp6_seq] = rtt;
      return 1;
    }else{
      return -1;
    }
}
