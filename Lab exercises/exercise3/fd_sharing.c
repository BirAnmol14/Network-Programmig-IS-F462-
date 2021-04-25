#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>

#define MAXBUF 256
void ReadAndExit(int sockfd,int fsize,int n,int offset);
void sendFD(int fd,int sockfd);
void die(const char * msg,int type){
  if(type==0){
    puts(msg);
    exit(0);
  }else{
    perror(msg);
    exit(1);
  }
}

int main(int argc,char * argv[]){
    if(argc<2||argc>2){
      die("Invalid usage: ./a.out <No of processes>",0);
    }
    int n = atoi(argv[1]);
    printf("Parent PID: %d\n",getpid());
    printf("N: %d\n",n);
    if(n<1){
      die("Enter correct process number",0);
    }
    printf("Enter filename: ");
    char fname[MAXBUF];
    memset(fname,'\0',MAXBUF);
    scanf("%s",fname);
    printf("Fname: %s\n",fname);
    int filefd = open(fname,O_RDONLY);
    if(filefd<0){
      die("Unable to open file",1);
    }
    struct stat st;
    int fsize=fstat(filefd,&st);
    if(fsize<0){
      die("Stat error",1);
    }
    fsize = st.st_size;
    printf("File size: %d\n",fsize);
    printf("File size/N => %d\n",fsize/n);
    int socks[n][2];
    int child[n];
    int i;
    for(i=0;i<n;i++){
      if (socketpair(PF_UNIX, SOCK_STREAM, 0, socks[i])<0){
          die("socketpair",1);
      }
      int pid = fork();
      if(pid == 0){
        for(int j=0;j<i;j++){
          close(socks[j][1]);
        }
        close(socks[i][1]);//close write end
        ReadAndExit(socks[i][0],fsize,n,i);
      }else{
          child[i]=pid;
          close(socks[i][0]);//close read end
      }
    }
    for(int i=0;i<n;i++){
        sendFD(filefd,socks[i][1]);
        waitpid(child[i],NULL,0);
        close(socks[i][1]);
    }
    close(filefd);
    puts("****** OVER ******");
}
void ReadAndExit(int sockfd,int fsize,int n,int offset){
  char c;
  struct iovec vector;
	struct msghdr msg;
  struct cmsghdr * cmsg;
  vector.iov_base = &c;
  vector.iov_len = 1;
  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &vector;
  msg.msg_iovlen = 1;
  int fd;
  cmsg = malloc(sizeof(struct cmsghdr) + sizeof(fd));
  cmsg->cmsg_len = sizeof(struct cmsghdr) + sizeof(fd);
  msg.msg_control = cmsg;
  msg.msg_controllen = cmsg->cmsg_len;
  if (recvmsg(sockfd, &msg, 0)<0){
        die("rcv msg",1);
  }
  memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
  close(sockfd);
  offset = offset * (fsize/n);
  lseek(fd,offset,SEEK_SET);
  char buf[fsize/n];
  printf("CHILD PID(%d) ",getpid());
  fflush(stdout);
  int r =read(fd,buf,fsize/n);
  printf("[%d bytes -- from offset: %d]: ",r,offset);
  fflush(stdout);
  write(fileno(stdout),buf,r);
  puts("");
  exit(0);
}
void sendFD(int fd,int sockfd){
  struct iovec vector;
  struct cmsghdr * cmsg;
  struct msghdr msg;
  char buf='1';//to send fd, at least 1 byte data must
  vector.iov_base = &buf;
  vector.iov_len = 1;
  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &vector;
  msg.msg_iovlen = 1;
  cmsg = malloc(sizeof(struct cmsghdr) + sizeof(fd));
  cmsg->cmsg_len = sizeof(struct cmsghdr) + sizeof(fd);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));
  msg.msg_control = cmsg;
  msg.msg_controllen = cmsg->cmsg_len;
  if (sendmsg(sockfd, &msg, 0) <0)
        die("Send Msg",1);
  close(sockfd);
  free(cmsg);
}
