#include<stdio.h>
#include<unistd.h>

int main ()
{
 // execl ("/bin/ls", "ls", "-l", (char *) 0);
 char * arr[]= {"ls","-l",NULL};
 execvp("ls",arr);
  printf ("hello");
}
