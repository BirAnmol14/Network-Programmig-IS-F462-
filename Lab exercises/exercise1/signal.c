/*
Name: Bir Anmol Singh
Id: 2018A7PS0261P
*/
#include <stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>
struct timeval t1;
int count=0;
void testAndKill();
sigset_t set;
void handle(int signo){
    if(signo==SIGALRM){
		sigprocmask(SIG_BLOCK,&set,NULL);
		int c=count;
		count=0;
		struct timeval t2;
		gettimeofday(&t2,NULL);
		printf("%d's SIGUSR1 count: %d (time: %ld)\n",getpid(),c,t2.tv_sec-t1.tv_sec);
		sigprocmask(SIG_UNBLOCK,&set,NULL);
        if(c==0){
            exit(0);
        }else{			
            testAndKill();
        }
    }else if(signo==SIGUSR1){
        count++;
    }
}
int main(int argc,char * argv[]) {
	int n;
	if(argc>=2){
			n = atoi(argv[1]);
			n=(int)n;
			if(n<0){
				printf("N must be >=0\n");
				exit(0);
			}
	}else{
		printf("Too few arguments\n");
		exit(0);
	}
	signal(SIGALRM,handle);
    signal(SIGUSR1,handle);
	sigemptyset(&set);
	sigaddset(&set,SIGUSR1);
	setvbuf(stdout, NULL, _IONBF, 0);
	
    printf("Process: %d\n",getpid());
    int pid=getpid();
    for(int i=0;i<n;i++){
            pid=fork();
            if(pid==0){
				int z=getpid()%13;
				printf("Process Created: %d (parent:%d) with z=%d\n",getpid(),getppid(),z);
				sleep(n*0.5);
                break;
            }else{
				sleep(0.5);
			}
			
    }
	if(pid==0){
		int z=getpid()%13;
		for(int i=0;i<z;i++){
			pid=fork();
			if(pid==0){
				printf("Process Created: %d (parent:%d)\n",getpid(),getppid());
				sleep((z+1)*0.5);
				break;
			}
			else{sleep(0.5);}
		}
    }
	sleep(3);
	gettimeofday(&t1,NULL);
	testAndKill();
    while(1);
    return 0;
}
void testAndKill(){
	int p=getpid();
	alarm(2);
    for(int i=1;i<=12;i++){
        if(kill(p+i,0)==0){
           kill(p+i,SIGUSR1);
        }
    }    
}
