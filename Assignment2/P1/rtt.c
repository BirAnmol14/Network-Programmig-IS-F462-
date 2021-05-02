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
#include <sys/epoll.h>
#include <pthread.h>

#define BUFSIZE 1500
#define PERHOSTLIM 3
#define TIMEOUT 3000

#define max(a,b)(a>b?a:b)
int MAXTHREAD = max((300/PERHOSTLIM),1);

typedef struct myRTT{
  char addr[40];//IP addr
  int ipv;//IP version
  float rtt;
  int sockfd;
  int seq;
  int rcv_cnt;
}myRTT;

typedef struct args{
  char addr[50];
  int num;
}Args;
pthread_mutex_t count = PTHREAD_MUTEX_INITIALIZER;
void * manage(void *);
int getIPVersion(const char *);
int getNextIP(FILE *,char * );
int getSocket(int);
unsigned short cal_chksum(unsigned short *, int);
void tv_sub(struct timeval *out, struct timeval *in);
int  send_packetV4(int seq,myRTT*,int num);
int  send_packetV6(int seq,myRTT*,int num);
void recv_packetV4(myRTT * r,int num);
int procV4(myRTT *,char *, int ,struct timeval *,int num);
void recv_packetV6(myRTT * r,int num);
int procV6(myRTT *,char *, int ,struct timeval *,int num);
void printStats(myRTT *,char *);

void die(const char * msg,int type){
  if(type==0){
    perror(msg);
    exit(1);
  }else{
    puts(msg);
  }
}

int totalThreads = 0;
int totalv4 = 0;
int totalv6 = 0;
int datalen = 56;
int main(int argc, char *argv[])
{
    if(argc<2||argc>2){
      puts("Invalid usage: ./a.out <IP filename name>");
      exit(1);
    }
    FILE * fp = fopen(argv[1],"r");
    if(fp==NULL){
        die("The input file does not exist",0);
    }
    puts("***** rtt program started *****");
    struct timeval start;
	gettimeofday(&start,NULL);
    int done = 0;
    while(!done){
        while(1){
            pthread_mutex_lock(&count);
              if(totalThreads>=MAXTHREAD){
                pthread_mutex_unlock(&count);
                continue;
              }
              break;
        }
        pthread_mutex_unlock(&count);
        char * addr= malloc(128*sizeof(char));
        memset(addr,'\0',128);
        int ret = getNextIP(fp,addr);
        if(!ret){
          done = 1;
          continue;
        }
        pthread_t t;
        Args * args = malloc(sizeof(Args)*1);
        strcpy(args->addr,addr);
        pthread_mutex_lock(&count);
        args->num = (totalv4+totalv6)*PERHOSTLIM;
        pthread_mutex_unlock(&count);
        int retr =pthread_create(&t,NULL,manage,args);
        if(retr== 0){
          pthread_mutex_lock(&count);
          totalThreads++;
          pthread_mutex_unlock(&count);
        }
        free(addr);

    }
    while(1){
        pthread_mutex_lock(&count);
        if(totalThreads==0){
          pthread_mutex_unlock(&count);
          break;
        }
        pthread_mutex_unlock(&count);
    }

   fclose(fp);
   puts("***** rtt program ended *****");
   struct timeval end;
   gettimeofday(&end,NULL);
   tv_sub(&end,&start);
   printf("Total %d (IPv4: %d + Ipv6: %d) IP addresses PINGED\n",totalv4+totalv6,totalv4,totalv6);
   int gap =end.tv_sec;
   int mins = gap/60;
   gap%=60;
   int sec = gap;
   printf("Total time taken=> %d mins: %d secs: %ld ms\n",mins,sec,(end.tv_usec)/1000);
   return 0;
}
void * manage(void * arg){
      Args * args = (Args *)arg;
      char * addr = args->addr;
      int num = args->num;
      pthread_detach(pthread_self());
      if(addr==NULL || strlen(addr)<=0){
        pthread_mutex_lock(&count);
        free(addr);
        totalThreads--;
        pthread_mutex_unlock(&count);
        pthread_exit(0);
      }

      int total = 0;
      int epfd;
      struct epoll_event ev;
      struct epoll_event evlist[PERHOSTLIM];
      pthread_mutex_lock(&count);
      epfd = epoll_create(3);
      if(epfd == -1){
        die("epoll error",0);
      }
      pthread_mutex_unlock(&count);
      int type = getIPVersion(addr);
      if(type<=0){
        pthread_mutex_lock(&count);
        totalThreads--;
        close(epfd);
        free(addr);
        pthread_mutex_unlock(&count);
        pthread_exit(0);
      }
      myRTT r[PERHOSTLIM];
      int closed[PERHOSTLIM];
      for(int i=0;i<PERHOSTLIM;i++){
        closed[i]=0;
      }
      pthread_mutex_lock(&count);
      if(type==4)
        totalv4++;
      else
        totalv6++;
      pthread_mutex_unlock(&count);
      for(int i=0;i<PERHOSTLIM;i++){
            memset(r[i].addr,'\0',40);
            strcpy(r[i].addr,addr);
            r[i].ipv = type;
            pthread_mutex_lock(&count);
            r[i].sockfd= getSocket(type);
            pthread_mutex_unlock(&count);
            r[i].rcv_cnt = 0;
            r[i].rtt = -1;
            r[i].seq = i;
            ev.events = EPOLLOUT;
            ev.data.ptr = &r[i];
            if (epoll_ctl (epfd, EPOLL_CTL_ADD, r[i].sockfd, &ev) == -1){die("epoll_ctl error",0);}
      }
      while(total!=PERHOSTLIM){

            int ready = epoll_wait (epfd, evlist, PERHOSTLIM, TIMEOUT);

            if (ready == -1)
            {
              if (errno == EINTR)
                continue;
              else
                //die("epoll_wait",1);
                break;
            }
            if(ready == 0){
              //timeout
              break;
            }
            for(int i=0;i<ready;i++){
                if(evlist[i].events & EPOLLOUT){
                    myRTT * r = (myRTT *)evlist[i].data.ptr;
                    //Send Packets
                    int ret;
                    if(r->ipv==4){
                        ret=send_packetV4(r->seq,r,num+r->seq);
                    }else{
                      ret=send_packetV6(r->seq,r,num+r->seq);
                    }
                    if(ret == 1){
                      ev.events = EPOLLIN;
                      ev.data.ptr = r;
                      if (epoll_ctl (epfd, EPOLL_CTL_MOD, r->sockfd, &ev) == -1){die("epoll_ctl error1",1);}
                    }
                    else{
                      ev.events = EPOLLOUT;
                      ev.data.ptr = r;
                      //if (epoll_ctl (epfd, EPOLL_CTL_DEL, r->sockfd, &ev) == -1){die("epoll_ctl error2",1);}
                      close(r->sockfd);
                      closed[r->seq]=1;
                      total++;
                    }
                }
                else if(evlist[i].events & EPOLLIN){
                    myRTT * r = (myRTT *)evlist[i].data.ptr;
                    //Recv Packets
                    if(r->ipv==4){
                        recv_packetV4(r,num+r->seq);
                    }else{
                      recv_packetV6(r,num+r->seq);
                    }
                    if(r->rcv_cnt>=1&&!closed[r->seq]){
                      ev.events = EPOLLIN;
                      ev.data.ptr = r;
                      //if (epoll_ctl (epfd, EPOLL_CTL_DEL, r->sockfd, &ev) == -1){die("epoll_ctl error",1);}
                      close(r->sockfd);
                      closed[r->seq] = 1;
                      total++;
                    }
                }
            }
      }
      pthread_mutex_lock(&count);
      for(int i=0;i<PERHOSTLIM;i++){
        if(closed[i]==0){
          close(r[i].sockfd);
          closed[i]=1;
        }
      }
      printStats(r,addr);
      totalThreads--;
      close(epfd);
      free(args);
      pthread_mutex_unlock(&count);
      pthread_exit(0);
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
int getNextIP(FILE * fp,char * msg){
  if(feof(fp)){
    return 0;
  }
  fscanf(fp,"%[^\n]",msg);
  fgetc(fp);
  if(strlen(msg)==0||feof(fp)){
    return 0;
  }
  return 1;
}
int getSocket(int ipv){
  if(ipv == 4){
    //IPv4
    int sock = socket(PF_INET,SOCK_RAW,IPPROTO_ICMP);
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
    int sock = socket(PF_INET6,SOCK_RAW,IPPROTO_ICMPV6);
    int hoplimit = 50;
    setsockopt(sock, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hoplimit,sizeof(hoplimit));
    if(sock <0 ){
      die("Socket failed",0);
    }
    struct icmp6_filter filt;
    ICMP6_FILTER_SETBLOCKALL(&filt);
    ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY,&filt);
    setsockopt(sock,IPPROTO_IPV6,ICMP6_FILTER,&filt,sizeof(filt));
    int size = 60*1024;
    setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size));
    int on=1;
    setsockopt(sock,SOL_SOCKET,IPV6_V6ONLY,&on,sizeof(on));
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

int send_packetV4(int seq,myRTT * r,int num)
{
    struct sockaddr_in other;
    other.sin_family = AF_INET;
    other.sin_addr.s_addr = inet_addr(r->addr);
    other.sin_port = htons(0);
   int sockfd = r->sockfd;
   int i, packsize;
   char sendpacket [BUFSIZE]={0};
   struct icmp *icmp;
   struct timeval *tval;
   icmp = (struct icmp*)sendpacket;
   icmp->icmp_type = ICMP_ECHO;
   icmp->icmp_code = 0;
   icmp->icmp_cksum = 0;
   icmp->icmp_seq = seq;
   icmp->icmp_id = htons(num);
   packsize = 8+datalen;
   memset(icmp->icmp_data,0xa5,datalen);
   tval = (struct timeval*)icmp->icmp_data;
   gettimeofday(tval, NULL);
   icmp->icmp_cksum = cal_chksum((unsigned short*)icmp, packsize);
   if (sendto(sockfd,sendpacket, packsize, 0,(struct sockaddr *)&other, sizeof(other)) < 0)
   {
       if(errno==EWOULDBLOCK){
           die("timeout",1);
           return -1;
       }
        die("sendto error",1);
        return -1;
   }
   return 1;
}
void recv_packetV4(myRTT * r,int num)
{
    struct sockaddr_in cli;
    char recvpacket [BUFSIZE];
    int n, clilen;
    float rtt;
    clilen = sizeof(cli);
    if ((n = recvfrom(r->sockfd, recvpacket, sizeof(recvpacket), 0, (struct sockaddr*) &cli, &clilen)) < 0)
        {
            if(errno==EWOULDBLOCK){
                die("timeout",1);
                r->rcv_cnt++;
                return;
            }
           die("recvfrom error",1);
           return;
         }
    struct timeval tvrecv;
    gettimeofday(&tvrecv, NULL);
    if (procV4(r,recvpacket, n,&tvrecv,num) < 0){
        //die("Failed to process packet",1);
        r->rcv_cnt = 0;
    }
}
int procV4(myRTT * r,char *recvbuf, int len,struct timeval * tvrecv,int num)
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

    if ((icmp->icmp_type == ICMP_ECHOREPLY) && (ntohs(icmp->icmp_id) == num))
    {
        if(icmplen < 16){
          printf("ICMP packets\'s length is less than 16, Not enough to be processed\n");
          return -1;
        }
        tvsend = (struct timeval*)icmp->icmp_data;
        tv_sub(tvrecv, tvsend);
        rtt = tvrecv->tv_sec * 1000.0+(tvrecv->tv_usec / 1000.0);
        r->rtt = rtt;
        r->rcv_cnt++;
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
int send_packetV6(int seq,myRTT * r,int num)
{
  struct sockaddr_in6 other;
  other.sin6_family = AF_INET6;
  inet_pton(AF_INET6,r->addr,&(other.sin6_addr));
  other.sin6_port = htons(0);
   int sockfd = r->sockfd;
   int i, packsize;
   char sendpacket [BUFSIZE];
   struct icmp6_hdr *icmp6;
   struct timeval *tval;
   icmp6 = (struct icmp6_hdr*)sendpacket;
   icmp6->icmp6_type = ICMP6_ECHO_REQUEST;
   icmp6->icmp6_code = 0;
   icmp6->icmp6_seq = seq;
   icmp6->icmp6_id = num;
   //checksum handled by kernel for us
   memset((icmp6+1),0xa5,datalen);
   tval = (struct timeval*)(icmp6+1);
   gettimeofday(tval, NULL);
   packsize = 8+datalen;
   if (sendto(sockfd,sendpacket, packsize, 0,(struct sockaddr *)&other, sizeof(other)) < 0)
   {
       if(errno==EWOULDBLOCK){
           die("timeout",1);
           return -1;
       }
        perror("sendto error");
        return -1;
   }
   return 1;
}
void recv_packetV6(myRTT * r,int num)
{
    struct sockaddr_in6 cli;
    int n, clilen;
    clilen = sizeof(cli);
    char recvpacket [BUFSIZE];
    if ((n = recvfrom(r->sockfd,recvpacket,sizeof(recvpacket),0,(struct sockaddr*)&cli,&clilen)) < 0)
        {
            if(errno==EWOULDBLOCK){
                die("timeout",1);
                r->rcv_cnt++;
                return;
            }
            printf("%s %d\n", r->addr,r->rcv_cnt);
           die("recvfrom error",1);
           return;
        }
    struct timeval tvrecv;
    gettimeofday(&tvrecv, NULL);
    if (procV6(r,recvpacket, n,&tvrecv,num) < 0){
        //die("Failed to process packet",1);
        r->rcv_cnt=0;
    }
}
int procV6(myRTT * r,char *recvbuf, int len,struct timeval * tvrecv,int num)
{
    double rtt;
    struct icmp6_hdr *icmp6;
    struct timeval *tvsend;
    icmp6 = (struct icmp6_hdr*)recvbuf;
    if(len<8){
      printf("ICMP packets\'s length is less than 8\n");
      return  -1;
    }
    if((icmp6->icmp6_type == ICMP6_ECHO_REPLY) && icmp6->icmp6_id == num){
      if(len < 16){
        printf("ICMP packets\'s length is less than 16, Not enough to be processed\n");
        return -1;
      }
      tvsend = (struct timeval*)(icmp6+1);
      tv_sub(tvrecv, tvsend);
      rtt = tvrecv->tv_sec * 1000.0+ (tvrecv->tv_usec / 1000.0);
      r->rtt = rtt;
      r->rcv_cnt++;
      return 1;
    }else{
      return -1;
    }
}

void printStats(myRTT * r,char * addr){
  printf("HOST(IPv%d): %-40s\t",r[0].ipv,addr);
  for(int i=0;i<PERHOSTLIM;i++){
    if(r[i].rtt==-1){
      printf("RTT(%d) = **** ms\t",i+1 );
    }else{
      printf("RTT(%d) = %.3f ms\t",i+1 ,r[i].rtt);
    }
  }
  puts("\n");
}
