#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#define key (2000)

int main ()
{
  int semid, i, j;
  pid_t ret;
  struct sembuf sb;

  semid = semget (key, 1, IPC_CREAT | 0666);

  if (semid >= 0)
    {
/*set sem value*/
      i = semctl (semid, 0, SETVAL, 1);
      if (i != -1)
	perror ("semctl setting sem value");

      ret = fork ();
      if (ret == 0)
	{fork();
	  for (i = 0; i < 10; i++)
	    {
	      sb.sem_num = 0;
	      sb.sem_op = -1;
	      sb.sem_flg = 0;
	      printf ("Child(%d): Attempting to acquire semaphore %d\n", getpid(),semid);
	      /* Acquire the semaphore */
	      if (semop (semid, &sb, 1) == -1)
		{
		  printf ("Child: semop failed.\n");
		  exit (-1);
		}
	      printf ("iChild: Semaphore acquired %d\n", semid);


	      printf ("Child: %d\n", i);
		sleep(1);

	/*      sb.sem_num = 0;
	      sb.sem_op = 1;
	      sb.sem_flg = 0;
	      printf ("Child:Releasing semaphore %d\n", semid);*/
	      /* Acquire the semaphore */
/*	      if (semop (semid, &sb, 1) == -1)
		{
		  printf ("Child: semop failed.\n");
		  exit (-1);
		}
	      printf ("Child: Semaphore released %d\n", semid);
*/
	    }

	  printf ("Child ends\n");
	}
      else
	{
	  printf ("Parent resumes.\n");
	  for (j = 0; j < 20; j++)
	    {

	/*      sb.sem_num = 0;
	      sb.sem_op = -1;
	      sb.sem_flg = 0;
	      printf ("Parent: Attempting to acquire semaphore %d\n", semid);*/
	      /* Acquire the semaphore */
/*	      if (semop (semid, &sb, 1) == -1)
		{
		  printf ("Parent: semop failed.\n");
		  exit (-1);
		}
	      printf ("Parent: Semaphore acquired %d\n", semid);
*/
	      printf ("Parent: %d\n", j);
		sleep(1);

	      sb.sem_num = 0;
	      sb.sem_op = 1;
	      sb.sem_flg = 0;
	      printf ("Parent: Releasing semaphore %d\n", semid);
	      /* Acquire the semaphore */
	      if (semop (semid, &sb, 1) == -1)
		{
		  printf ("Parent: semop failed.\n");
		  exit (-1);
		}
	      printf ("Parent: Semaphore released %d\n", semid);
	      
	    }
		wait(NULL);
		wait(NULL);
	}
    }

  else
    perror ("semget:");

  return 0;
}
