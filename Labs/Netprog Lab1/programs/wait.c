#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
int main ()
{
  int i = 0, j = 0; 
  pid_t ret;
  int status;

  ret =fork ();
  if (ret == 0)
    {
      for (i = 0; i < 50; i++)
        printf ("Child: %d\n", i);
      printf ("Child ends\n");
      //exit(0); must if using vfork
    }
  else
    {
    // wait (&status);//if vfork used, not needed.. wait till any child returns
    //waitpid(ret,NULL,0);//0 means blocking call .. wait till a specific child return
   // waitpid(ret,NULL,WNOHANG); does not block immediately if child not available
      printf ("Parent resumes.\n");
      for (j = 0; j < 50; j++)
        printf ("Parent: %d\n", j);
    }
}
