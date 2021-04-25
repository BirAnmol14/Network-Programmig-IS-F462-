#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     
#include <signal.h>
#include <sys/wait.h>
#define MAXPENDING 5    

void DieWithError(char *errorMessage);  
void HandleTCPClient(int clntSocket);   
void handle(int signo){
	if(signo ==SIGCHLD){
		wait(NULL);
	}
}
int main(int argc, char *argv[])
{
    signal(SIGCHLD,handle);
    int servSock;                    
    int clntSock;                    
    struct sockaddr_in echoServAddr; 
    struct sockaddr_in echoClntAddr; 
    unsigned short echoServPort;     
    unsigned int clntLen;            

    if (argc != 2)     
    {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  

    
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");
      
    
    memset(&echoServAddr, 0, sizeof(echoServAddr));   

    echoServAddr.sin_family = AF_INET;                    

    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    echoServAddr.sin_port = htons(echoServPort);      

    
    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

    
    if (listen(servSock, MAXPENDING) < 0)
        DieWithError("listen() failed");
    printf("Listen fd PID: %d\n",getpid());
    for (;;)
    {
        
        clntLen = sizeof(echoClntAddr);

        
        if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, 
                               &clntLen)) < 0)
            DieWithError("accept() failed");

        int pid=fork();
	if(pid==0){
		close(servSock);
        	printf("Process %d is Handling client %s:(%d)\n", getpid(),inet_ntoa(echoClntAddr.sin_addr),ntohs(echoClntAddr.sin_port));
		HandleTCPClient(clntSock);
        	printf("Client %s:(%d) Left, Process(%d) exit\n",inet_ntoa(echoClntAddr.sin_addr),ntohs(echoClntAddr.sin_port),getpid());
		exit(1);
	}
	close(clntSock);
    }
    
}
