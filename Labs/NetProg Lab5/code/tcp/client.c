#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     
#include <sys/select.h>
#define RCVBUFSIZE 512   

void DieWithError(char *errorMessage);  
int max(int a,int b){
	return a>b?a:b;
}
int main(int argc, char *argv[])
{
    int sock;                        
    struct sockaddr_in echoServAddr; 
    unsigned short echoServPort;     
    char *servIP;                    
    char *echoString;                
    char echoBuffer[RCVBUFSIZE];     
    unsigned int echoStringLen;      
    int bytesRcvd, totalBytesRcvd;   

    if ((argc < 3) || (argc > 4))        {
       fprintf(stderr, "Usage: %s <Server IP> <Echo Word> [<Echo Port>]\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];             
    echoString = argv[2];         

    if (argc == 4)
        echoServPort = atoi(argv[3]); 
    else
        echoServPort = 7;  

    
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    
    memset(&echoServAddr, 0, sizeof(echoServAddr));     

    echoServAddr.sin_family      = AF_INET;                     
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   
    echoServAddr.sin_port        = htons(echoServPort); 

    
    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connect() failed");

    echoStringLen = strlen(echoString);          

    
    if (send(sock, echoString, echoStringLen, 0) != echoStringLen)
        DieWithError("send() sent a different number of bytes than expected");

    totalBytesRcvd = 0;

    printf("Received: ");                
    while (totalBytesRcvd < echoStringLen)
    {
        
        if ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) <= 0)
            DieWithError("recv() failed or connection closed prematurely");
        totalBytesRcvd += bytesRcvd;   
        echoBuffer[bytesRcvd] = '\0';  
        printf("%s", echoBuffer);      
    }

    printf("\n");    
	
    printf("User Control\n");
    printf("Enter data: (Cntrl+d for eof)\n");
    fd_set rset;
    FD_ZERO(&rset);
    int stdineof = 0;
    while(1){
	    if(!stdineof){
	    FD_SET(fileno(stdin),&rset);
	    }
    	    FD_SET(sock,&rset);
	    int maxf = max(sock,fileno(stdin))+1;	    
	    select(maxf,&rset,NULL,NULL,NULL);
	    if(FD_ISSET(sock,&rset)){
		    //data available on Socket
		    printf("Server Sent: ");
		   int n= recv(sock,echoBuffer,RCVBUFSIZE-1,0);
		   if(n==0){
			   if(stdineof){
			   	//normal termination
				break;
			   }else{
			   	//abnormal termination
				printf("Server closed unexpectedly\n");
				exit(1);
			   }
		    }
	            echoBuffer[n]='\0';		   
		    printf("%s \n",echoBuffer);
	    }else if(FD_ISSET(fileno(stdin),&rset)){
		int n = read(fileno(stdin),echoBuffer,RCVBUFSIZE-1);
		if(n==0){
			stdineof=1;
			shutdown(sock,SHUT_WR);//partial close to socket for write end
			FD_CLR(fileno(stdin),&rset);
			continue;
		}
		echoBuffer[n] = '\0';
		send(sock,echoBuffer,n,0);//send the string to server
            }
    }
    printf("\nBye Bye!\n");
    close(sock);
    exit(0);
}
