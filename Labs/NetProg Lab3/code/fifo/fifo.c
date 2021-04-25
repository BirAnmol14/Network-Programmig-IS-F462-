#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <signal.h>
#include <sys/wait.h>
#define MSGSIZE 32
void handle(int signo){
	if(signo==SIGUSR1){
		return;
	}
}
int main ()
{
	signal(SIGUSR1,handle);
	int i;
	int rfd,wfd;
	if(mkfifo("fifo",O_CREAT|O_EXCL|0666)<0)
		if(errno=EEXIST)
			perror("fifo");
	pid_t ret;
	ret = fork ();
	char inbuff[MSGSIZE];
	if (ret > 0)
	{
		rfd=open("fifo", O_RDONLY,0);
		wfd=open("fifo",O_WRONLY|O_NONBLOCK,0);
		i = 0;
		while (i < 10)
		{
			write (wfd,"Hello, how are you\0?", MSGSIZE);
			kill(ret,SIGUSR1);
			pause();
			read (rfd, inbuff, MSGSIZE);
			printf ("child: %s\n", inbuff);
			i++;
		}
		exit(1);
	}
	else
	{
		
		wfd=open("fifo",O_WRONLY|O_NONBLOCK,0);
		rfd = open("fifo",O_RDONLY,0);
		i = 0;
		while (i < 10)
		{
			sleep(1);
			read (rfd, inbuff, MSGSIZE);
			printf ("parent: %s\n", inbuff);
			write (wfd, "i am fine\0",MSGSIZE);
			i++;
			kill(getppid(),SIGUSR1);
			pause();
		}
		exit(0);
	}
	wait(NULL);
	unlink("fifo");
	exit (0);
}
