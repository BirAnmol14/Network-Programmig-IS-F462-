/*TCPServerwithSelectNBIO.c*/  
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/fcntl.h>
  
#define LISTENQ 15
#define MAXLINE 80
#define EXPECTED_LEN 10
  struct clientstate
{
  
   
  char buf[MAXLINE];
   
   
 

processClient (struct clientstate *cs)
{
  
  
    



{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
    {
      
      
      
      
    
  
  
  
  
    
    {
      
      
      
      
	
	{			/* new client connection */
	  
	  
		int fl=fcntl(connfd, F_GETFL, 0);
		fl=fl|O_NONBLOCK;
		fcntl(connfd, F_SETFL, &fl);
	  
		   
		   
	  
	    
	      
	      {
		
		
		
	      
	  
	    {
	      
	      
	    
	  
	  
	    
	  
	    
	  
	    
	
      
	
	{			/* check all clients for data */
	  
	    
	  
		   client[i].in_index, client[i].out_index, client[i].buf);
	  
	    {
		printf("Writing\n");
	      
		      write (sockfd, client[i].buf + client[i].out_index,EXPECTED_LEN);
		
		
		  
		  
		    {
		      
		      
			client[i].in_index=0;
			client[i].out_index=0;
		      
		      
		    
	
	    
	  
	       && FD_ISSET (sockfd, &rset))
	    
	    {
	      
		    read (sockfd, client[i].buf + client[i].in_index,
			  MAXLINE)) == 0)
		
		{
		  
		    /*connection closed by client */ 
		    close (sockfd);
		  
		  
		  
		
	      
	      else
		{
		  
		  
		  
		    {
		      
		      
		      
		    
		  
		    //write (sockfd, buf, n);
		}
	      
		
	    
	
      
	
	  
	  {			/* check all clients for data */
	    
	      
	    
	      {
		
		
		
		
	      
	  
    


