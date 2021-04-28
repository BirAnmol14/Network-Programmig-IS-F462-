#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

typedef struct ServerChild {
    pid_t pid;
    int pipe;
    int status;
    int requests;
} ServerChild;

typedef struct ServerArray {
    ServerChild** arr;
    int size;
    int capacity;
} ServerArray;

void errExit(char* msg, int perr) {
    if(perr) {
        perror(msg);
    } else {
        fprintf(stderr, "%s\n", msg);
    }
    exit(EXIT_FAILURE);
}

// GLOBAL VARIABLES
int minSpareServers;
int maxSpareServers;
int maxRequestsPerChild;
int numServers = 0;
int numIdleServers = 0;

ServerArray* s = NULL;

// -------------------------------------------------------------------
// DYNAMIC ARRAY OPERATIONS

ServerArray* newServerArray(int capacity) {
    ServerArray* s = malloc(sizeof(ServerArray));
    s->capacity = capacity;
    s->size = 0;
    s->arr = (ServerChild**) malloc(sizeof(ServerChild*) * capacity);
    for(int i = 0; i < capacity; i++) {
        s->arr[i] = NULL;
    }
    return s;
}

void changeArrayCap(ServerArray* s, int newCapacity) {
    if(s->size > newCapacity)
        return;
    ServerChild** newArr = (ServerChild**) malloc(sizeof(ServerChild*) * newCapacity);
    for(int i = 0; i < newCapacity; i++) {
        if(i < s->size)
            newArr[i] = s->arr[i];
        else
            newArr[i] = NULL;
    }
    free(s->arr);
    s->arr = newArr;
    s->capacity = newCapacity;
}

int addServer(ServerArray* s, ServerChild* c) {
    if(s->size == s->capacity) 
        changeArrayCap(s, s->capacity * 2);
    s->size++;
    s->arr[s->size - 1] = c;
    return s->size - 1;
}

void removeDeadServers(ServerArray* s) {
    int j = 0;
    for(int i = 0; i < s->size; i++) {
        if((s->arr[i])->status == -1) {
            free(s->arr[i]);
        } else {
            s->arr[j++] = s->arr[i];
        }
    }
    s->size = j;
    if(s->size < s->capacity / 4)
        changeArrayCap(s, s->capacity / 2);
}

ServerChild* get(ServerArray* s, int i) {
    return s->arr[i];
}

// -------------------------------------------------------------------
// CHILD FUNCTIONS

void serviceConnection(int connsock) {
    // receive HTTP request
    char buffer[256];
    int bytesRead = 0;
    while(bytesRead = recv(connsock, buffer, 256, 0)) {
        if(bytesRead == -1)
            errExit("recv", 1);
        if(bytesRead < 256)
            break;
    }

    sleep(1);

    // send dummy response
    char* dummyResponse = "HTTP/1.1 200 OK\r\n\r\n";
    int respLen = strlen(dummyResponse);
    if(send(connsock, dummyResponse, respLen, 0) == -1)
        errExit("send", 1);
    
    close(connsock);
}

void childMain(int pipe, int lsock) {
    int epfd = epoll_create(2);
    if(epfd == -1)
        errExit("epoll_create", 1);

    struct epoll_event evpipe;
    evpipe.events =  EPOLLIN;
    evpipe.data.fd = pipe;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, pipe, &evpipe) == -1)
        errExit("epoll_ctl", 1);

    // EPOLLEXCLUSIVE flag handles thundering herd problem
    // EPOLLIN event delivered to only one process
    struct epoll_event evsock;
    evsock.events = EPOLLIN | EPOLLEXCLUSIVE;
    // evsock.events = EPOLLIN;
    evsock.data.fd = lsock;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, lsock, &evsock) == -1)
        errExit("epoll_ctl", 1);
    
    struct epoll_event evlist[2];

    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    int numRequests = 0;

    while(1) {
        int epoll_ret = epoll_wait(epfd, evlist, 2, -1);
        if(epoll_ret == -1) {
            if(errno == EINTR)
                continue;
            else
                errExit("epoll_wait", 1);
        }
        for(int i = 0; i < epoll_ret; i++) {
            if(evlist[i].events != EPOLLIN) continue;
            if(evlist[i].data.fd == lsock) {
                // new connection maybe available
                int connsock = accept(lsock, (struct sockaddr*) &clientAddr, &clientAddrLen);
                if(connsock == -1)
                    continue;

                // connection established
                char ipStr[20] = {0};
                inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, 20);
                int port = ntohs(clientAddr.sin_port);
                printf("[%d]: Accepted Connection (%s:%d)\n", getpid(), ipStr, port);

                if(send(pipe, "C", 1, 0) == -1)
                    errExit("send", 1);
                serviceConnection(connsock);
                if(send(pipe, "F", 1, 0) == -1)
                    errExit("send", 1);

                numRequests++;
                if(numRequests == maxRequestsPerChild) {
                    // dont listen for any new connections
                    if(epoll_ctl(epfd, EPOLL_CTL_DEL, lsock, NULL) == -1)
                        errExit("epoll_ctl", 1);
                    close(lsock);
                }
            }
        }
        for(int i = 0; i < epoll_ret; i++) {
            if(evlist[i].events != EPOLLIN) continue;
            if(evlist[i].data.fd == pipe) {
                char buffer[1];
                int read_ret = read(pipe, buffer, 1);
                if(read_ret == -1)
                    errExit("read", 1);
                else if(read_ret == 0) {
                    errExit("Parent process terminated", 0);
                }
                if(buffer[0] == 'K') {
                    // kill command sent by parent
                    // printf("[%d]: Exiting\n", getpid());
                    exit(EXIT_SUCCESS);
                }
            }
        }
    }
}

// -------------------------------------------------------------------
// PARENT FUNCTIONS

char* serverIP = "127.0.0.1";
int serverPort = 31415;

int initializeListenSock(void) {
    int lsock = socket(PF_INET, SOCK_STREAM, 0);
    if(lsock == -1)
        errExit("socket", 1);

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr));
    serverAddr.sin_port = htons(serverPort);
    if(bind(lsock, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) == -1)
        errExit("bind", 1);
    
    if(listen(lsock, 10) == -1)
        errExit("listen", 1);
    
    return lsock;
}

void sigHandle(int signo) {
    if(signo == SIGCHLD) {
        pid_t pid;
        while((pid = waitpid(-1, NULL, WNOHANG)) > 0);

    } else if(signo == SIGINT) {
        printf("[P]: %d processes, %d clients\n", numServers, numServers - numIdleServers);
        for(int i = 0; i < s->size; i++) {
            ServerChild* c = get(s, i);
            char* status;
            if(c->status == 0)
                status = "Idle";
            else if(c->status == 1)
                status = "Busy";
            else
                status = "Dead";
            printf("[%d]: (%s) handled %d clients\n", c->pid, status, c->requests);
        }
    }
}

void modifyInterestList(int epfd, int op, int fd, ServerChild* c) {
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = (void*) c;
    if(epoll_ctl(epfd, op, fd, &ev) == -1)
        errExit("epoll_ctl", 1);
}

void spawnServerChild(ServerArray* s, int epfd, int lsock) {
    int pipe[2];
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, pipe) == -1)
        errExit("socketpair", 1);
    int fork_ret = fork();
    if(fork_ret == -1)
        errExit("fork", 1);
    else if(fork_ret == 0) {
        signal(SIGINT, SIG_IGN);
        // child
        close(pipe[0]);
        // NOTE: may want to free master data structures
        childMain(pipe[1], lsock);
        errExit("Server child returned unexpectedly", 0);
    } else {
        // parent 
        close(pipe[1]);
        ServerChild* c = malloc(sizeof(ServerChild));
        c->pid = fork_ret;
        c->pipe = pipe[0];
        c->status = 0;
        c->requests = 0;
        addServer(s, c);
        modifyInterestList(epfd, EPOLL_CTL_ADD, c->pipe, c);
        numServers++;
        numIdleServers++;
    }
}

void spawnServers(ServerArray* s, int epfd, int lsock) {
    int spawnRate = 1;
    while(numIdleServers < minSpareServers) {
        for(int i = 0; i < spawnRate; i++) {
            spawnServerChild(s, epfd, lsock);
        }
        sleep(1);
        if(spawnRate < 32)
            spawnRate *= 2;
    }
    printf("[P]: (spawn) %d processes, %d clients, %d idle\n", numServers, numServers - numIdleServers, numIdleServers);
}

void killServerChild(ServerChild* c, int epfd, int sendK) {
    if(sendK && send(c->pipe, "K", 1, 0) == -1)
        errExit("send", 1);
    modifyInterestList(epfd, EPOLL_CTL_DEL, c->pipe, NULL);
    numServers--;
    if(c->status == 0)
        numIdleServers--;
    c->status = -1;
}

void killServers(ServerArray* s, int epfd) {
    for(int i = s->size - 1; i >= 0; i--) {
        if(numIdleServers <= maxSpareServers)
            break;
        ServerChild* c = get(s, i);
        if(c->status == 0) {
            // kill idle server
            killServerChild(c, epfd, 1);
        }
    }
    removeDeadServers(s);
    printf("[P]: (kill) %d processes, %d clients, %d idle\n", numServers, numServers - numIdleServers, numIdleServers);
}

// -------------------------------------------------------------------
// MAIN

int main(int argc, char* argv[]) {
    if(argc != 4) {
        fprintf(stderr, "Usage: %s <minSpareServers> <maxSpareServers> <maxRequestsPerChild>\n", argv[0]);
        exit(EXIT_SUCCESS);
    }
    minSpareServers = atoi(argv[1]);
    maxSpareServers = atoi(argv[2]);
    maxRequestsPerChild = atoi(argv[3]);
    printf("minSpareServers: %d\nmaxSpareServers: %d\nmaxRequestsPerChild: %d\n", minSpareServers, maxSpareServers, maxRequestsPerChild);

    int lsock = initializeListenSock();    

    s = newServerArray(minSpareServers);
    int epfd = epoll_create(minSpareServers);
    if(epfd == -1)
        errExit("epoll_create", 1);
    int maxEvents = 5;
    struct epoll_event evlist[maxEvents];
    int deadServersFlag;

    signal(SIGCHLD, sigHandle);
    signal(SIGINT, sigHandle);

    while(1) {
        if(numIdleServers < minSpareServers)
            spawnServers(s, epfd, lsock);
        if(numIdleServers > maxSpareServers)
            killServers(s, epfd);
        
        int epoll_ret = epoll_wait(epfd, evlist, maxEvents, -1);
        if(epoll_ret == -1) {
            if(errno == EINTR)
                continue;
            else
                errExit("epoll_wait", 1);
        }

        deadServersFlag = 0;
        for(int i = 0; i < epoll_ret; i++) {
            if(evlist[i].events != EPOLLIN) continue;
            ServerChild* c = (ServerChild*) evlist[i].data.ptr;
            char buffer[1];
            int read_ret = read(c->pipe, buffer, 1);
            if(read_ret == -1)
                errExit("read", 1);
            else if(read_ret == 0) {
                printf("Server child terminated unexpectedly\n");
                killServerChild(c, epfd, 0);
                deadServersFlag = 1;
            }
            if(buffer[0] == 'C') {
                // new connection accepted
                c->status = 1;
                c->requests++;
                numIdleServers--;
            } else if(buffer[0] == 'F') {
                // finished servicing requests from connection
                if(c->requests == maxRequestsPerChild) {
                    // recycle child process
                    killServerChild(c, epfd, 1);
                    deadServersFlag = 1;
                    spawnServerChild(s, epfd, lsock);
                    printf("[P]: (recycle) %d processes, %d clients, %d idle\n", numServers, numServers - numIdleServers, numIdleServers);
                } else {
                    c->status = 0;
                    numIdleServers++;
                }
            }
        }
        
        if(deadServersFlag)
            removeDeadServers(s);
    }
}