#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
void handle(int signo){
	if(signo==SIGCHLD){
		wait(NULL);
	}
	if(signo==SIGUSR1){

	}
}
int main ()
{
  int id,i;
  key_t key;
  int pid;
  int shmid;
  char *data;
  signal(SIGUSR1,handle);
  signal(SIGCHLD,handle);
  key = ftok ("shmget.c", 'R');

  if ((shmid = shmget (key, 1024, 0644 | IPC_CREAT)) == -1)
    {
      perror ("shmget: shmget failed");
      exit (1);
    }
  data = shmat (shmid, (void *) 0, 0);

  if (data == (char *) (-1))
    perror ("shmat");
pid =fork();
  if(pid==0)
{

for(i=0;i<50;i++)
{	printf("%s\n", data);
	sprintf(data, "I am child and my pid is %d and my count is %d\n", getpid(), i);
	kill(getppid(),SIGUSR1);
	pause();
}
shmdt(data);
exit(0);
}
else if(pid>0)
{
	
for(i=0;i<50;i++)
{	pause();
	printf("%s\n", data);
	sprintf(data, "I am parent and my pid is %d and my count is %d\n", getpid(), i);
	if(i==49){break;}
	kill(pid,SIGUSR1);
} 
}
printf("%s\n",data);
shmdt(data);
int j=shmctl(shmid,IPC_RMID,NULL);
if(j<0){printf("Failed to remove memory\n");}
  return 0;
}
