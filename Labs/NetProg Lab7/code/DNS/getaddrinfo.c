#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
int main()
{
	int n;
struct addrinfo hints, *res, *ressave;
bzero(&hints, sizeof(struct addrinfo));
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_DGRAM;
while(1){
	char host[128];
	memset(host,'\0',128);
	printf("Enter Hotname: ");
	scanf("%s",host);
	if ( (n = getaddrinfo(host, NULL, NULL, &res)) != 0)
	{ 
		printf("tcp_connect error for %s: %s\n",host, gai_strerror(n));
		exit(0);
	}
	ressave = res;
	while(res!= NULL){
	int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0)
		continue; // ignore this one 
	if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
		break; // success 
	close(sockfd); // ignore this one 
		res = res->ai_next;
	}
	char ad[512];
		memset(ad,'\0',512);
		if(res->ai_family == AF_INET6){
			printf("%s\n",inet_ntop(res->ai_family,res->ai_addr,ad,res->ai_addrlen));
		}
		else if(res->ai_family == AF_INET){
			struct sockaddr_in * sa = (struct sockaddr_in *) res->ai_addr;
			printf("%s\n",inet_ntoa(sa->sin_addr));
		}
			
	freeaddrinfo(ressave);
	
}
}
