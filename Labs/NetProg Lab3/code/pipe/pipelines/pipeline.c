#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
int main ()
{
  int i;
  int p[2];
  pid_t ret;
  pipe (p);
  ret = fork ();

  if (ret == 0)
    {
	    int p1[2];
	    pipe(p1);
	int pid = fork();
	if(pid==0){
		printf("LS: \n");
		dup2(p1[1],1);
		/*
      		close (1);//close the stdout 
      		dup (p[1]);//copy to write end of pipe
      			*/
      		close (p1[0]);//you do not need read end
      		execlp ("ls", "ls", "-l", NULL);
	}else{
		wait(NULL);
		printf("GREP: \n");
		/* One read wirte end won't work as there will never be EOF
		dup2(p[1],1);
		dup2(p[0],0);
		*/ 
		dup2(p1[0],0);//read from pipe 1
		close(p1[1]);//you do not need the write end of this pipe
		dup2(p[1],1);//write to pipe 0
		close(p[0]);//you don't need this read end
		execlp("grep","grep","^d",NULL);
	}
    }

  if (ret > 0)
    {
	    wait(NULL);
	    	printf("WC: \n");
      		dup2(p[0],0);//read from pipe close stdin
      		close (p[1]);//you do not need write end plus EOF detected
      		   		//wait not needed as read is a blcoking call
      		execlp ("wc", "wc", "-l", NULL);

    }
}
