#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#define MAXTHREAD 2
int thread_cnt=0;
pthread_mutex_t cnt = PTHREAD_MUTEX_INITIALIZER;
void str_echo(int sockfd)
{
	ssize_t n;
	char buf[1000];
	while((n=read(sockfd,buf,1000))>0)
	{
	write(sockfd,buf,n);
	printf("%s\n",buf);
	}
	pthread_mutex_lock(&cnt);
	thread_cnt--;
	pthread_mutex_unlock(&cnt);
}

static void	*doit(void *);		/* each thread executes this function */
int main(int argc, char **argv)
{
	int listenfd, connfd;
	socklen_t addrlen, len;
	pthread_t tid1;
	struct sockaddr_in cliaddr;
	struct sockaddr_in servaddr;
	listenfd=socket(AF_INET,SOCK_STREAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(atoi(argv[1]));
	bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	listen(listenfd,MAXTHREAD);
	for ( ; ; ) {
		len = addrlen;
		connfd = accept(listenfd,(struct sockaddr *) &cliaddr, &len);
		pthread_mutex_lock(&cnt);
		thread_cnt++;
		if(thread_cnt<=MAXTHREAD){
			printf("Connection from: %s:%d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
			pthread_create(&tid1, NULL, &doit, (void *) connfd);
		}else{
			struct linger s;
			s.l_onoff = 1;
			s.l_linger = 0;
			setsockopt(connfd,SOL_SOCKET,SO_LINGER,&s,sizeof(s));
			close(connfd);
			puts("Max Limit reached, Aborted connection");
			thread_cnt--;
		}
		pthread_mutex_unlock(&cnt);
	}
}

static void *doit(void *arg)
{
	pthread_detach(pthread_self());
	str_echo((int) arg);	/* same function as before */
	close((int) arg);		/* we are done with connected socket */
	return(NULL);
}

