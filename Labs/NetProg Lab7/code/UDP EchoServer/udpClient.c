#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
int main(int argc,char * argv[]){
	if(argc <2 || argc >2) {
		puts("Add <Server Port> in args");
		exit(1);
	}
	int port = atoi (argv[1]);
	struct sockaddr_in server;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	int len = sizeof(server);
	int sock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (sock < 0){puts("Socket failed");exit(1);}
	int pkSize = 2;
	while(1){
		char * c = malloc(pkSize*sizeof(char));
		memset(c,'a',pkSize-1);
		c[pkSize] = '\0';
		printf("Sending PkSIZE: %d\n",pkSize);
		int ret = sendto(sock,c,strlen(c),0,(struct sockaddr *)&server,len);
		if(ret<0) {
				puts("Sending Failed");
				break;
		}	
		pkSize *=2;
		ret = recvfrom(sock,c,pkSize,0,(struct sockaddr *)&server,&len);
		if(ret<0){
			puts("recv failed");exit(1);
		}
		free(c);
	}
	printf("Max Sent till now: %d\n",pkSize/2);
	int prev = pkSize/2;
	int next = pkSize;
	pkSize = (next+prev)/2;
	for(int i = 0 ; i<15 ;i++){
		char * c = malloc(pkSize*sizeof(char));
		memset(c,'a',pkSize-1);
		c[pkSize] = '\0';
		printf("Sending PkSIZE: %d\n",pkSize);
		int ret = sendto(sock,c,strlen(c),0,(struct sockaddr *)&server,len);
		if(ret<0) {
				puts("Sending Failed");
				next = pkSize; 
		}else{	
			prev = pkSize;
		}
		pkSize = (prev+next)/2;
		if(ret < 0){continue;}
	       	else{
	  		ret = recvfrom(sock,c,pkSize,0,(struct sockaddr *)&server,&len);
			if(ret<0){
				puts("recv failed");exit(1);
			}
			free(c);
		}
	}
	puts("Theoretical UDP MAX reached");
	printf("%d --  %d  -- %dBytes\n",prev,pkSize,next);
	close(sock);
}

