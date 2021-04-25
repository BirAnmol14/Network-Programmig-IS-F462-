/*TCPServerwithSelect.c*/
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <sys/select.h>
#include <netdb.h>
#include  <arpa/inet.h>
#include <string.h>
#define LISTENQ 15
#define MAXLINE 80
#define MAXMSGS 20
typedef struct messages{
	int sock;
	char ** msgs;
}Message;
int main (int argc, char **argv)
{ if(argc < 2 ){
   puts("Usage: ./a.out <PORT>");
   exit(1);
  }
  int i, maxi, maxfd, listenfd, connfd, sockfd;
  int nready, client[FD_SETSIZE];
  ssize_t n;
  fd_set rset, allset;
  char buf[MAXLINE];
  socklen_t clilen;
  struct sockaddr_in cliaddr, servaddr;

  listenfd = socket (AF_INET, SOCK_STREAM, 0);

  bzero (&servaddr, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port = htons (atoi(argv[1]));

  bind (listenfd, (struct sockaddr *) & servaddr, sizeof (servaddr));
  listen (listenfd, LISTENQ);

  Message his[LISTENQ];
  for(int i=0;i<LISTENQ;i++){
	his[i].sock = -1;
	his[i].msgs = malloc(MAXMSGS * sizeof(char *));
	for(int j=0;j<MAXMSGS;j++){
		his[i].msgs[j] = NULL;
	}
  }
  int cons = -1;
  maxfd = listenfd;		/* initialize */
  maxi = -1;			/* index into client[] array */
  for (i = 0; i < FD_SETSIZE; i++)
    client[i] = -1;		/* -1 indicates available entry */
  FD_ZERO (&allset);
  FD_SET (listenfd, &allset);
	
  for (;;)
    {
      rset = allset;		/* structure assignment */
      nready = select (maxfd + 1, &rset, NULL, NULL, NULL);

      if (FD_ISSET (listenfd, &rset))
	{			/* new client connection */
	  clilen = sizeof (cliaddr);
	  connfd = accept (listenfd, (struct sockaddr *) & cliaddr, &clilen);
	  printf ("new client: %s, port %d\n",
		  inet_ntoa ( cliaddr.sin_addr),
		  ntohs (cliaddr.sin_port));
	  cons++;
	  his[cons].sock = connfd;
	  for (i = 0; i < FD_SETSIZE; i++)
	    if (client[i] < 0)
	      {
		client[i] = connfd;	/* save descriptor */
		break;
	      }
	  if (i == FD_SETSIZE){

	    printf ("too many clients");
		exit(0);
	}
	  FD_SET (connfd, &allset);	/* add new descriptor to set */
	  if (connfd > maxfd)
	    maxfd = connfd;	/* for select */
	  if (i > maxi)
	    maxi = i;		/* max index in client[] array */

	  if (--nready <= 0)
	    continue;		/* no more readable descriptors */
	}

      for (i = 0; i <= maxi; i++)
	{ memset(buf,'\0',MAXLINE);			/* check all clients for data */
	  if ((sockfd = client[i]) < 0)
	    continue;
	  if (FD_ISSET (sockfd, &rset))
	    {
	      if ((n = read (sockfd, buf, MAXLINE)) == 0)
		{
		  /*connection closed by client */
		  close (sockfd);
		  FD_CLR (sockfd, &allset);
		  client[i] = -1;
		  //clear up the DS as well
		}
	      else{
		 int pos = -1;
		for(int i=0;i<LISTENQ;i++){
			if(his[i].sock == sockfd){
				pos = i;
				break;	
			}		
		}
		if(pos !=-1 ){
			int last = 0;
			for(int i=0;i<MAXMSGS;i++){
				if(his[pos].msgs[i] == NULL){
					his[pos].msgs[i] = malloc((n+1)*sizeof(char));
					memset(his[pos].msgs[i],'\0',n+1);
					strcpy(his[pos].msgs[i],buf);
					last =1;
				}
				write(sockfd,his[pos].msgs[i],strlen(his[pos].msgs[i]));
				if(last){break;}
			}
		}
		//write (sockfd, buf, n);
	      }

	      if (--nready <= 0)
		break;		/* no more readable descriptors */
	    }
	}
    }
}

