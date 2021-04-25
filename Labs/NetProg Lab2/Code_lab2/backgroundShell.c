#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <error.h>
#include <signal.h>
#define MAX 10
void pr_exit (int status);
int main (int argc, char *argv[])
{
  signal(SIGCHLD,pr_exit);
  pid_t pid[MAX];
  int i, status;
  for (i = 1; i < argc; i++)
    {
      if ((pid[i] = fork ()) < 0)
        perror ("fork error");
      else if (pid[i] == 0)
        {
          printf ("Executing %s \n", argv[i]);
          execlp (argv[i], argv[i], (char *) 0);
          perror ("error in execlp");
        }
    }
  while(1);
}


void pr_exit (int status)
{
	if(status==SIGCHLD){
		wait(&status);
	}
  if (WIFEXITED (status))
    printf ("normal termination, exit status = %d\n", WEXITSTATUS (status));
  else if (WIFSIGNALED (status))
    printf ("abnormal termination, signal number = %d \n", WTERMSIG (status));
  else if (WIFSTOPPED (status))
    printf ("child stopped, signal number = %d\n", WSTOPSIG (status));
}
