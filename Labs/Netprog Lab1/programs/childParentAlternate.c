#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
void handle(int);
int main(){
	int pid =fork();
	signal(SIGUSR1,handle);
	if(pid==0){
		printf("Child Pid:%d\n",getpid());
		int i=0;
		do{
			pause();
			printf("Child: %d\n",i+1);
			i++;
			kill(getppid(),SIGUSR1);
		}while(i<50);
	}else{
		printf("Parent Pid:%d\n",getpid());
		for(int i=0;i<50;i++){
			printf("Parent: %d\n",i+1);
			kill(pid,SIGUSR1);
			pause();
		}
	}
}
void handle(int signo){
	
}
