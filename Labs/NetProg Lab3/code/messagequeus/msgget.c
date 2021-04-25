#include <stdio.h> 
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/ipc.h>   
#include <sys/msg.h>   
#include <unistd.h>
int
main(){
int queue_id = msgget(5678, IPC_CREAT|IPC_EXCL|0600); 
if (queue_id == -1) {
    perror("msgget");
    exit(1);
}
}
