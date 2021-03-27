cclient: msgq_client.c
	gcc -o client msgq_client.c

cserver: msgq_server.c
	gcc -o server msgq_server.c

compile: cclient cserver

rclient: compile
	./client

rserver: server
	./server
