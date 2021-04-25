#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

typedef struct{
	int dpid;
	int spid;
	char chat[100];
	}Message;

int
main ()
{
  int i, fd, fd1,n;
  char buf[100];
  char fifoname[30], fifonamecl[30];
	Message incoming;


  i = mkfifo ("/tmp/chatfifoserver", 0666);
  if (i < 0)
    {
      if (errno == EEXIST)
	{
	  printf ("errno is set as %d\n", errno);
	}
	  perror ("mkfifo");
    }

  fd = open ("/tmp/chatfifoserver", O_RDONLY);

  while ((n = read (fd, &incoming, sizeof(Message))) > 0)
    {

sprintf(fifonamecl,"/tmp/%d",incoming.dpid);
  fd1=open(fifonamecl,O_WRONLY);
	write(fd1,&incoming,sizeof(Message));
kill(incoming.dpid,SIGUSR1);
printf("relayed message from %d to %d\n", incoming.spid, incoming.dpid);
 	
    }

  unlink (fifoname);

}
