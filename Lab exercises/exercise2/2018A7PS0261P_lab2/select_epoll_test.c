#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <sys/select.h>
#include <sys/epoll.h>

#define MAXSOCK 1000
#define TIMELIM 5
#define MAXBUF 256
#define PORT 12345
#define SERVaddr "127.0.0.1"

int done = 0;
void Select(int n);
void Epoll(int n);
void TCPclient(int n);
void TCPSelectServer(int child[],int n);
void TCPEpollServer(int child[],int n);
int getRand(int limit);
void printDiffTime(struct timeval * start);

void die(const char * msg,int type){
  if(type==0){
    puts(msg);
    exit(1);
  }else{
    perror(msg);
    exit(1);
  }
}

int main(int argc,char * argv[]){
  if(argc<2||argc>2){
      die("Illegal Usage: ./a.out <No of Process>",0);
  }
  int n = atoi(argv[1]);
  if(n<=0 || n>MAXSOCK){
    die("N is not a valid number of child processes",0);
  }
  puts("Program Starting\n");
  Select(n);
  puts("");
  Epoll(n);
  puts("\n***** END *****");
}

void Select(int n){
    puts("*** SELECT TIMINGS ***");
    int child[n];
    for(int i=0;i<n;i++){
        int pid = fork();
        if(pid == 0){
          TCPclient(n);
        }else{
          child[i]=pid;
        }
    }
    TCPSelectServer(child,n);
    puts("*** SELECT OVER ***");
}
void Epoll(int n){
  puts("*** EPOLL TIMINGS ***");
  int child[n];
  for(int i=0;i<n;i++){
      int pid = fork();
      if(pid == 0){
        TCPclient(n);
      }else{
        child[i]=pid;
      }
  }
  TCPEpollServer(child,n);
  puts("*** Epoll OVER ***");
}
int getRand(int limit){
  srand(time(NULL));
  return rand()%limit;
}
void TCPclient(int n){
    sleep(TIMELIM);
    int *fds;
    int total = MAXSOCK;
    fds = malloc(total*sizeof(int));
    int curr = 0;
    struct sockaddr_in serv;
    serv.sin_port=htons(PORT);
    serv.sin_addr.s_addr = inet_addr(SERVaddr);
    serv.sin_family = AF_INET;
    while(!done){
        if(curr<total){
          //create new socket
          fds[curr] = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
          if(fds[curr]<0){
            die("Sock err",1);
          }
          int ret = connect(fds[curr],(struct sockaddr *)&serv,sizeof(serv));
          if(ret<0){
            if(errno == EINTR){
              if(done){
                break;
              }
            }
            die("Connect failed",1);
          }
          curr++;
        }
        int i = getRand(curr);
        char * buff = malloc(MAXBUF * sizeof(char));
        memset(buff,'\0',MAXBUF);
        strcpy(buff,"Hello");
        int ret = send(fds[i],buff,strlen(buff)+1,0);
        if(ret<0){
          if(errno==EINTR){
            if(done){
              break;
            }
          }else{
            die("Send err",1);
          }
        }
        memset(buff,'\0',MAXBUF);
        ret = recv(fds[i],buff,MAXBUF,0);
        if(ret<0){
          if(errno==EINTR){
            if(done){
              break;
            }
          }else{
            die("recv err",1);
          }
        }
    }
    for(int i=0 ; i<curr;i++){
        close(fds[i]);
    }
    free(fds);
    exit(0);
}


void TCPSelectServer(int * child, int n){
    printf("Starting things in %d seconds\n",TIMELIM);
    int total =0;
    int connfd,sockfd;
    char * buff = malloc(MAXBUF*sizeof(char));
    memset(buff,'\0',MAXBUF);
    int listenfd = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(listenfd<0){
      die("sock err",1);
    }
    struct sockaddr_in serv,cli;
    int clilen;
    serv.sin_port=(htons(PORT));
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_family = AF_INET;
    int yes=1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
    int ret = bind(listenfd,(struct sockaddr *)&serv,sizeof(serv));
    if(ret < 0){
      die("bind failed",1);
    }
    ret = listen(listenfd,MAXSOCK+1);
    if(ret<0){
      die("listen failed",1);
    }
  struct timeval start;
  int first = 1;
  int client[MAXSOCK+1];
  int maxfd = listenfd;
  int maxi = -1;
  for (int i = 0; i < MAXSOCK+1; i++){client[i] = -1;}
  fd_set allset,rset;
  FD_ZERO (&allset);
  FD_SET (listenfd, &allset);
  while(1){
      rset = allset;
      int nready = select (maxfd + 1, &rset, NULL, NULL, NULL);
      if (FD_ISSET (listenfd, &rset))
    	{			//new client
          if(first){
            gettimeofday(&start,NULL);
            first = 0;
          }
	        clilen = sizeof (cli);
	        connfd = accept (listenfd, (struct sockaddr *) &cli, &clilen);
          total++;
          if(total==10||total ==100|| total >= 1000){
              printf("%d Client Connections: ",total);
              printDiffTime(&start);
          }
          if(total >= 1000){
            for(int i=0;i<n;i++){
              kill(child[i],SIGINT);
            }
            break;
          }
	  	    for(int i = 0; i < MAXSOCK+1; i++){
	           if (client[i] < 0)
	            {
		              client[i] = connfd;
                  if(i>maxi){maxi = i;}
		               break;
	            }
            }
	          FD_SET (connfd, &allset);
            if (connfd > maxfd){maxfd = connfd;}

    	  if (--nready <= 0)
    	    continue;
	  }
    for (int i = 0; i <= maxi; i++)
  	{
          if ((sockfd = client[i]) < 0)continue;
  	      if (FD_ISSET (sockfd, &rset))
  	      {
              int n =recv(sockfd,buff,MAXBUF,0);
              if(n<0){
                die("recv err",1);
              }
              if(n==0){
                close(sockfd);
                continue;
              }
              memset(buff,'\0',MAXBUF);
              strcpy(buff,"Hi");
              n=send(sockfd,buff,strlen(buff)+1,0);
              if(n<0){
                die("send err",1);
              }
  	       }
  	}

  }
  for(int i=0;i<=maxi;i++){
    close(client[i]);
  }
  free(buff);
  for(int i=0;i<n;i++){
    waitpid(child[i],NULL,0);
  }
  close(listenfd);
}

void TCPEpollServer(int child[],int n){
      printf("Starting things in %d seconds\n",TIMELIM);
      int total =0;
      int connfd,sockfd;
      char * buff = malloc(MAXBUF*sizeof(char));
      memset(buff,'\0',MAXBUF);
      int listenfd = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
      if(listenfd<0){
        die("sock err",1);
      }
      struct sockaddr_in serv,cli;
      int clilen;
      serv.sin_port=(htons(PORT));
      serv.sin_addr.s_addr = htonl(INADDR_ANY);
      serv.sin_family = AF_INET;
      int yes=1;
      setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
      int ret = bind(listenfd,(struct sockaddr *)&serv,sizeof(serv));
      if(ret < 0){
        die("bind failed",1);
      }
      ret = listen(listenfd,MAXSOCK+1);
      if(ret<0){
        die("listen failed",1);
      }
    struct timeval start;
    int first = 1;
    int client[MAXSOCK+1];

    int epfd = epoll_create (MAXSOCK+1);
    struct epoll_event ev;
    struct epoll_event evlist[MAXSOCK+1];
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    if (epoll_ctl (epfd, EPOLL_CTL_ADD, listenfd, &ev) == -1){die("epoll_ctl",1);}
    int ready;
    int end=0;
    while(!end){
      ready = epoll_wait (epfd, evlist, MAXSOCK+1, -1);
      if (ready <0)
	     {
	         if (errno == EINTR)
	             continue;
	         else
	           die("epoll_wait",1);
	     }
       for(int i=0;i<ready;i++){
         if(evlist[i].events & EPOLLIN){
           if(evlist[i].data.fd == listenfd){
             if(first){
               gettimeofday(&start,NULL);
               first = 0;
             }
             int connfd = accept(listenfd,(struct sockaddr *)&cli,&clilen);
             client[total] = connfd;
             total++;
             ev.events = EPOLLIN;	/* Only interested in input events */
       		   ev.data.fd = connfd;
       		   if (epoll_ctl (epfd, EPOLL_CTL_ADD, connfd, &ev) == -1){die("epoll_ctl",1);}
             if(total==10||total ==100|| total >= 1000){
                 printf("%d Client Connections: ",total);
                 printDiffTime(&start);
             }
             if(total >= 1000){
               for(int i=0;i<n;i++){
                 kill(child[i],SIGINT);
               }
               end=1;
               break;
             }
           }else{
             int sockfd = evlist[i].data.fd;
             int n =recv(sockfd,buff,MAXBUF,0);
             if(n<0){
               die("recv err",1);
             }
             if(n==0){
               close(sockfd);
               continue;
             }
             memset(buff,'\0',MAXBUF);
             strcpy(buff,"Hi");
             n=send(sockfd,buff,strlen(buff)+1,0);
             if(n<0){
               die("send err",1);
             }
           }
         }
       }
    }
    for(int i=0;i<total;i++){
        close(client[i]);
    }
    free(buff);
    for(int i=0;i<n;i++){
      waitpid(child[i],NULL,0);
    }
    close(epfd);
    close(listenfd);
}
void printDiffTime(struct timeval * start){
  struct timeval end;
  gettimeofday(&end,NULL);
  if ((end.tv_usec -= start->tv_usec) < 0)
   {
       --end.tv_sec;
       end.tv_usec += 1000000;
   }
   end.tv_sec -= start->tv_sec;
   printf("%.3f ms time taken\n",end.tv_sec * 1000.0+(end.tv_usec / 1000.0));
}
