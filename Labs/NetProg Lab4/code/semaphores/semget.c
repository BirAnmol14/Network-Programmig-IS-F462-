#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "semun.h"
#include <errno.h>
#define key (2000)
int main ()
{ 
  struct semid_ds ds;
 union semun args;
  args.buf = &ds;
  int id;
  id = semget (key, 1, IPC_CREAT | 0666);
  if (id < 0)
    printf ("Semaphore is not created\n");
  int val = semctl(id,0,GETVAL);
  printf("Semaphore successfully created with id %d:%d\n", id,val);  
  /*int i=0;
  while(i<5){
	  printf("Test %d\n",i+1);
	  i++;
  int test = semctl(id,0,IPC_STAT,args);
 if(test==-1){perror("Error in semaphore");exit(1);}
if(ds.sem_otime!=0){break;}
  }*/
  //if(ds.sem_otime==0){
	  //semaphore was not initialised previously
  semctl(id,0,SETVAL,6);
  val=semctl(id,0,GETVAL);
  printf("Semaphore value set to: %d\n",val);
  //}
  if(semctl(id,0,IPC_RMID)==EPERM){
	printf("Failed to remove semaphore\n");
  }  else{
	printf("Semaphore removed\n");
	val = semctl(id,0,GETVAL);
	if(val<0){
		printf("surely...\n");
	}
  }

return 0;
}
