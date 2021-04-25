#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Practical.h"

int main(int argc, char *argv[]) {

  if (argc < 3 || argc > 4) // Test for correct number of arguments
    DieWithUserMessage("Parameter(s)",
        "<Server Address/Name> <Echo Word> [<Server Port/Service>]");

  char *server = argv[1];     // First arg: server address/name
  char *echoString = argv[2]; // Second arg: word to echo

  size_t echoStringLen = strlen(echoString);
  if (echoStringLen > MAXSTRINGLENGTH) // Check input length
    DieWithUserMessage(echoString, "string too long");

  // Third arg (optional): server port/service
  char *servPort = (argc == 4) ? argv[3] : "echo";

  // Tell the system what kind(s) of address info we want
  struct addrinfo addrCriteria;                   // Criteria for address match
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = AF_UNSPEC;             // Any address family
  // For the following fields, a zero value means "don't care"
  addrCriteria.ai_socktype = SOCK_DGRAM;          // Only datagram sockets
  addrCriteria.ai_protocol = IPPROTO_UDP;         // Only UDP protocol

  // Get address(es)
  struct addrinfo *servAddr; // List of server addresses
  int rtnVal = getaddrinfo(server, servPort, &addrCriteria, &servAddr);
  if (rtnVal != 0)
    DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));

  // Create a datagram/UDP socket
  int sock = socket(servAddr->ai_family, servAddr->ai_socktype,
      servAddr->ai_protocol); // Socket descriptor for client
  if (sock < 0)
    DieWithSystemMessage("socket() failed");

  puts("Before connect");
  struct sockaddr local,peer;
  int len= sizeof(struct sockaddr);
  getsockname(sock,&local,&len);
  getpeername(sock,&peer,&len);
  PrintSocketAddress(&local,stdout);
  puts("");
  PrintSocketAddress(&peer,stdout);
  puts("");
//Adding connect 
 int r=connect(sock,servAddr->ai_addr,servAddr->ai_addrlen);
 if(r<0){DieWithSystemMessage("Connect() failed");}
 puts("After Connect"); 
 getsockname(sock,&local,&len);
  getpeername(sock,&peer,&len);
  PrintSocketAddress(&local,stdout);
  puts("");
  PrintSocketAddress(&peer,stdout);
  puts("");
  // Send the string to the server
  ssize_t numBytes =/* sendto(sock, echoString, echoStringLen, 0,
      servAddr->ai_addr, servAddr->ai_addrlen);*/
	  send(sock,echoString,echoStringLen,0);
  if (numBytes < 0)
    DieWithSystemMessage("sendto() failed");
  else if (numBytes != echoStringLen)
    DieWithUserMessage("sendto() error", "sent unexpected number of bytes");

  // Receive a response

  struct sockaddr_storage fromAddr; // Source address of server
  // Set length of from address structure (in-out parameter)
  socklen_t fromAddrLen = sizeof(fromAddr);
  char buffer[MAXSTRINGLENGTH + 1]; // I/O buffer
  numBytes =/* recvfrom(sock, buffer, MAXSTRINGLENGTH, 0,
      (struct sockaddr *) &fromAddr, &fromAddrLen);*/ recv(sock,buffer,MAXSTRINGLENGTH,0);
  if (numBytes < 0)
    DieWithSystemMessage("recvfrom() failed");
  else if (numBytes != echoStringLen)
    DieWithUserMessage("recvfrom() error", "received unexpected number of bytes");

  // Verify reception from expected source
 /* if (!SockAddrsEqual(servAddr->ai_addr, (struct sockaddr *) &fromAddr))
    DieWithUserMessage("recvfrom()", "received a packet from unknown source");
*/
  freeaddrinfo(servAddr);
  puts("Before disconnect");
  getsockname(sock,&local,&len);
  getpeername(sock,&peer,&len);
  PrintSocketAddress(&local,stdout);
  puts("");
  PrintSocketAddress(&peer,stdout);
  puts("");
struct sockaddr sa;
sa.sa_family = AF_UNSPEC;
r = connect(sock,&sa,sizeof(sa));
if(r<0){DieWithSystemMessage("Disconnect Error");}
puts("After disconnect");
  getsockname(sock,&local,&len);
  getpeername(sock,&peer,&len);
  PrintSocketAddress(&local,stdout);
  puts("");
  PrintSocketAddress(&peer,stdout);
  puts("");
  buffer[echoStringLen] = '\0';     // Null-terminate received data
  printf("Received: %s\n", buffer); // Print the echoed string

  close(sock);
  exit(0);
}
