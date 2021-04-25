#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char *argv[])
{
  int i;
  char **ptr;
  extern char **environ;

  for (i = 0; i < argc; i++)    /* echo all command-line args */
    printf ("argv[%d]: %s\n", i, argv[i]);

  for (ptr = environ; *ptr != 0; ptr++) /* and all env strings */
    printf ("%s\n", *ptr);
  printf("\n\n");
  setenv("HELLO","Testing",1);
  putenv("NAME=Bir");
  for (ptr = environ; *ptr != 0; ptr++) /* and all env strings */
    printf ("%s\n", *ptr);
  exit (0);
  //Changes made to environ reverted once process gets over
}
