#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#define MSGSIZE 16
void handle(int signo){
	if(signo==SIGUSR1){
		
	}
}
int main ()
{
  signal(SIGUSR1,handle);
  int i;
  char *msg = "How are you?";
  char inbuff[MSGSIZE];
  int p[2];
  pid_t ret;
  pipe (p);
  ret = fork ();
  if (ret > 0)
    {
      i = 0;
      while (i < 10)
        {
          write (p[1], msg, MSGSIZE);
         // sleep (2);
	 kill(ret,SIGUSR1);
	  pause();
          read (p[0], inbuff, MSGSIZE);
          printf ("Parent: %s\n", inbuff);
          i++;
        }
    exit(1);
    }
  else
    {
      i = 0;
      while (i < 10)
        {
          //sleep (1);
          read (p[0], inbuff, MSGSIZE);
          printf ("Child: %s\n", inbuff);
	  kill(getppid(),SIGUSR1);
          write (p[1], "i am fine", MSGSIZE);
	  pause();
          i++;
        }
    }
  exit (0);
}
