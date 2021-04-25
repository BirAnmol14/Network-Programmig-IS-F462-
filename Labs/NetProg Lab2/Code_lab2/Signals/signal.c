#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
void printAll(sigset_t *);
void int_handler(int);
int p;
int main()
{
  //signal (SIGINT, SIG_DFL); default
  //signal (SIGINT, SIG_IGN); sigint and sigstp can't be ignored
 //signal (SIGINT, int_handler);
 signal(SIGUSR1,int_handler);
 sigset_t set;
 sigfillset(&set);
 sigdelset(&set,SIGINT);
 sigdelset(&set,SIGSTOP);
 p=fork();
 if(p==0){
 while(1){sleep(5);printf("generating SIGUSR1\n");kill(getppid(),SIGUSR1);}
 exit(0);
 }
 signal(SIGINT,int_handler);
  printf ("Entering infinite loop\n");
  while (1)
    {
      sigprocmask(SIG_BLOCK,&set,NULL);
      sleep (10);
      //sigprocmask(SIG_UNBLOCK,&set,NULL);
      printAll(&set);
    }
  printf (" This is unreachable\n");
}
void printAll(sigset_t * s){
	sigset_t s1;
	if(sigpending(&s1)==0){
		if(sigismember(&s1,SIGUSR1)){
				printf("Signal Sigusr1 %d was pending\n",SIGUSR1);
		}
	}
}

void int_handler(int signo){
	if(signo==SIGUSR1){
		printf("Received\n");return;
	}
	printf("running\n");
	kill(p,SIGINT);
	exit(0);
}

