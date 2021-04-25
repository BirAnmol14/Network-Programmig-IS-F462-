#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
int
main(int argc,char **argv)
{
	int	sd;
	struct	sockaddr_in server;
	
	sd = socket (AF_INET,SOCK_STREAM,0);
	
	server.sin_family = AF_INET;
	//server.sin_addr.s_addr=inet_addr(argv[1]);
	//inet_aton(argv[1],&server.sin_addr);
	inet_pton(AF_INET, argv[1], &server.sin_addr);
	server.sin_port = htons(12345);
	int n=connect(sd, (struct sockaddr*) &server, sizeof(server));
	if(n<0){exit(1);}
	printf("Connected to server\n");
        for (;;) {
	   send(sd, "HI", 2,0 );
           sleep(2);
        }
	return 0;
}
