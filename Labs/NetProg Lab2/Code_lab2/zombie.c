#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
void handle(int signo){
	if(signo==SIGCHLD){
		printf("\nCleaning Zombie\n");
		wait(NULL);
	}
}
int
main()
{
pid_t pid;
signal(SIGCHLD,handle);//could have ignored signal as well
pid=fork();

if(pid<0)
        perror("fork:");
else if(pid==0){
        printf("In Child: pid = %d, parent pid= %d", getpid(), getppid());
        printf("child finishing...");
        exit(0);
}
else if (pid>0){
	int status;
//wait(&status); -simple way without handler	
while(1);
}

}
