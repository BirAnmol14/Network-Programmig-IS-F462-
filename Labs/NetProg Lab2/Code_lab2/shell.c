#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
int main(int argc,char * argv[]){
	char *arr[argc];
	int enter=0;
	for(int i=1;i<argc;i++){
		enter =1;
		printf("%s\n",argv[i]);
		arr[i-1]=argv[i];	
	}
	
	if(!enter){
		printf("Give more arguments\n");exit(1);
	}
	arr[argc-1]=NULL;
	int pid =fork();
	if(pid==0){
		execvp(arr[0],arr);
		printf("execvp error\n");
	}
	else{
		wait(NULL);
		printf("Shell over\n");
	}
}
