#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#define MAX_NODES 256
#define LISTEN_QSIZE 10
#define SERVREQ_MSG_BUFFLEN 512
#define SERVRESP_MSG_BUFFLEN 2048
#define RESP_MSG_BUFFLEN 2048

typedef struct Job {
    int node;   
    int commandLength;
    char* command;
} Job;

typedef struct Request {
    int numJobs;
    char user[33];
    Job* jobs;
    int state;
    int isBroadcast;
} Request;

void closeRequest(Request* req);

int num_nodes;
uint32_t ip_map[MAX_NODES + 1];

int serverPort = 31415;
int lsock = -1;

void errExit(char* msg, int printErrNo) {
    if(printErrNo)
        perror(msg);
    else
        fprintf(stderr, "%s\n", msg);
    fprintf(stderr, "Exiting...\n");
    exit(EXIT_FAILURE);
}

void initializeServer(char* configFilePath) {
    // load config file
    FILE* configFile = fopen(configFilePath, "r");
    char ipaddr[16];
    int i = 1;
    num_nodes = 0;
    while(fgets(ipaddr, 16, configFile) != NULL) {
        ipaddr[strcspn(ipaddr, "\n")] = 0;
        printf("%s\n", ipaddr);
        if(i > MAX_NODES) {
            fprintf(stderr, "ERROR: Maximum nodes supported: %d\n", MAX_NODES);
            exit(EXIT_FAILURE);
        }
        num_nodes++;
        inet_pton(AF_INET, ipaddr, &ip_map[i++]);
    }

    // create socket
    lsock = socket(PF_INET, SOCK_STREAM, 0);
    if(lsock == -1) 
        errExit("socket", 1);
    // bind to serverPort
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(serverPort);
    if(bind(lsock, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) == -1)
        errExit("bind", 1);
    // start listening
    if(listen(lsock, LISTEN_QSIZE) == -1)
        errExit("listen", 1);
    printf("Listening on port %d.\n", serverPort);
    return;
}

int matchAddrWithNode(struct sockaddr_in* clientAddr) {
    for(int i = 1; i <= num_nodes; i++) {
        if(ip_map[i] == clientAddr->sin_addr.s_addr)
            return i;
    }
    return -1;
}

int registerRequest(char* message, int messageLength, int node, Request* requestList) {
    Request req;
    char* token;
    char* delim = " \n";
    token = strtok(message, delim);
    if(token == NULL || strcmp(token, "REQ") != 0)
        return -1;
    req.numJobs = atoi(strtok(NULL, delim));
    if(req.numJobs <= 0)
        return -1;
    req.jobs = malloc(sizeof(Job) * req.numJobs);
    token = strtok(NULL, delim);
    if(!token) return -1;
    strncpy(req.user, token, strlen(token));
    req.user[strlen(token)] = 0;
    // printf("%d %s\n", req.numJobs, req.user);
    for(int i = 0; i < req.numJobs; i++) {
        token = strtok(NULL, delim);
        if(!token) return -1;
        printf("a");
        req.jobs[i].node = atoi(token);
        token = strtok(NULL, delim);
        if(!token) return -1;
        printf("a");
        req.jobs[i].commandLength = atoi(token);
        token = strtok(NULL, "\n");
        if(!token) return -1;
        printf("a");
        req.jobs[i].command = malloc(req.jobs[i].commandLength + 1);
        strncpy(req.jobs[i].command, token, req.jobs[i].commandLength);
        req.jobs[i].command[req.jobs[i].commandLength] = 0;
        // printf("%d %d %s\n", req.jobs[i].node, req.jobs[i].commandLength, req.jobs[i].command);
    }
    req.isBroadcast = 0;
    req.state = -1;
    if(requestList[node].state != -2)
        return -1;
    if(req.numJobs == 1 && req.jobs[0].node == -1 && strcmp(req.jobs[0].command, "nodes") != 0) {
        // broadcast request
        req.isBroadcast = 1;
        req.numJobs = num_nodes;
        int commandLength = req.jobs[0].commandLength;
        char* command = req.jobs[0].command;
        free(req.jobs);
        req.jobs = malloc(sizeof(Job) * num_nodes);
        for(int i = 1; i <= num_nodes; i++) {
            req.jobs[i-1].node = i;
            req.jobs[i-1].commandLength = commandLength;
            req.jobs[i-1].command = command;
        }
    }
    requestList[node] = req;
    return 0;    
}

void serviceRequest(int clientNode, Request* requestList, int* csock, char* outputBuffer, int outputLength) {
    Request* req = &requestList[clientNode];
    if(req -> state == -2) {
        // client disconnected, request dropped
        closeRequest(req);
    }
    if(strcmp((req -> jobs)[0].command, "nodes") == 0) {
        char messageBuffer[RESP_MSG_BUFFLEN];
        int messageLength = sprintf(messageBuffer, "RESP 0 ");
        char ipStr[17];
        for(int i = 1; i <= num_nodes; i++) {
            inet_ntop(AF_INET, &ip_map[i], ipStr, 16);
            if(csock[i] != -1)
                messageLength += sprintf(messageBuffer + messageLength, "[n%d] (%s): Active\n", i, ipStr);
            else
                messageLength += sprintf(messageBuffer + messageLength, "[n%d] (%s): Unavailable\n", i, ipStr);
        }
        int messageLengthBuff = htonl(messageLength);
        send(csock[clientNode], &messageLengthBuff, 4, 0);
        send(csock[clientNode], messageBuffer, messageLength, 0);
        closeRequest(req);
        return;
    }
    int remoteNode; // last remote node
    if(req -> state == -1)
        remoteNode = 0;
    else
        remoteNode = (req -> jobs)[req -> state].node;
    req -> state = req -> state + 1;
    if(req -> state == req -> numJobs) {
        // request serviced, send response to clientNode
        char messageBuffer[RESP_MSG_BUFFLEN];
        int messageLength = sprintf(messageBuffer, "RESP %d ", remoteNode);
        messageLength += sprintf(messageBuffer + messageLength, "%s", outputBuffer);
        int messageLengthBuff = htonl(messageLength);
        send(csock[clientNode], &messageLengthBuff, 4, 0);
        send(csock[clientNode], messageBuffer, messageLength, 0);
        closeRequest(req);
        return;
    }
    // service next job in request
    Job* j = &(req -> jobs)[req -> state];
    if(csock[j -> node] == -1) {
        // node unavailable, send unavailable response
        char messageBuffer[RESP_MSG_BUFFLEN];
        int messageLength = sprintf(messageBuffer, "RESP %d Unavailable", j -> node);
        int messageLengthBuff = htonl(messageLength);
        send(csock[clientNode], &messageLengthBuff, 4, 0);
        send(csock[clientNode], messageBuffer, messageLength, 0);
        if(req -> isBroadcast && j -> node != req -> numJobs) {
            // exec for next node
            serviceRequest(clientNode, requestList, csock, NULL, 0);
        } else {
            // invalid request
            closeRequest(req);
        }
        return;
    }
    char messageBuffer[SERVREQ_MSG_BUFFLEN];
    int messageLength = sprintf(messageBuffer, "SREQ %d %s ", clientNode, req -> user);
    messageLength += sprintf(messageBuffer + messageLength, "%s\n", j->command);
    if(outputLength > 0) {
        char eot = 0x04;
        messageLength += sprintf(messageBuffer + messageLength, "%s\n", outputBuffer);
    }
    int messageLengthBuff = htonl(messageLength);
    send(csock[j->node], &messageLengthBuff, 4, 0);
    send(csock[j->node], messageBuffer, messageLength, 0);
    printf("Sent request to remote client for service (job %d)\n", req->state);
}

void closeRequest(Request* req) {
    req -> state = -2;
    if(req -> isBroadcast) {
        free((req -> jobs)[0].command);
    } else {
        for(int i = 0; i < req -> numJobs; i++) {
            free((req -> jobs)[i].command);
        }
    }
    req -> numJobs = 0;
    free(req -> jobs);
    return;
}

char* receiveJobResponse(char* message, int messageLength, int* outputLength, int* clientNode) {
    char* token;
    token = strtok(message, " ");
    if(token == NULL || strcmp(token, "SRSP") != 0)
        return NULL;
    token = strtok(NULL, " ");
    if(!token) return NULL;
    *clientNode = atoi(token);

    char* outputBuffer = token + strlen(token) + 1;
    *outputLength = messageLength - (outputBuffer - message);
    outputBuffer[*outputLength] = 0;

    return outputBuffer;
}

int main(int argc, char* argv[]) {
    char* configFilePath = "config.txt";
    if(argc == 2) {
        if(strcmp(argv[1], "--help") == 0) {
            printf("Usage:\n%s\n%s <path to config file>\n", argv[0], argv[0]);
            exit(EXIT_SUCCESS);
        } else {
            configFilePath = argv[1];
        }
    }
    initializeServer(configFilePath);

    int csock[num_nodes+1];
    Request requestList[num_nodes+1];
    for(int i = 0; i <= num_nodes; i++) {
        csock[i] = -1;
        requestList[i].state = -2;
    }

    int maxfd;
    fd_set readSet;
    while(1) {
        FD_ZERO(&readSet);
        FD_SET(lsock, &readSet);
        maxfd = lsock;
        for(int i = 1; i <= num_nodes; i++) {
            if(csock[i] != -1) {
                FD_SET(csock[i], &readSet);
                if(maxfd < csock[i]) maxfd = csock[i];
            }
        }
        int sr = select(maxfd + 1, &readSet, NULL, NULL, NULL);
        if(sr == -1)
            errExit("select", 1);
        if(sr > 0 && FD_ISSET(lsock, &readSet)) {
            printf("New connection request.\n");
            fflush(stdout);
            // new connection
            struct sockaddr_in clientAddr;
            int clientAddrLength = sizeof(clientAddr);
            int connfd = accept(lsock, (struct sockaddr*) &clientAddr, &clientAddrLength);
            if(connfd == -1)
                errExit("accept", 1);
            int node = matchAddrWithNode(&clientAddr);
            if(node == -1) {
                // unknown node
                close(connfd);
            } else if(csock[node] != -1) {
                // existing connection with node
                close(connfd);
            } else {
                // accept connection
                csock[node] = connfd;
                printf("Connection Request from node %d accepted...\n", node);
            }
        }
        for(int i = 1; sr > 0 && i <= num_nodes; i++) {
            if(csock[i] == -1) continue;
            if(FD_ISSET(csock[i], &readSet)) {
                // new message
                int messageLength;
                int bytesRecvd = recv(csock[i], &messageLength, 4, 0);
                if(bytesRecvd == 0) {
                    // client disconnected
                    csock[i] = -1;
                    requestList[i].state = -2;
                    char message[10];
                    int messageLength = sprintf(message, "CLOS %d", i);
                    int messageLengthBuff = htonl(messageLength);
                    for(int j = 1; j <= num_nodes; j++) {
                        if(csock[j] != -1) {
                            send(csock[j], &messageLengthBuff, 4, 0);
                            send(csock[j], message, messageLength, 0);
                        }
                    }
                    printf("Node %d disconnected.\n", i);
                    continue;
                }
                messageLength = ntohl(messageLength);
                printf("ML: %d\n", messageLength);
                char message[messageLength];
                if(recv(csock[i], message, messageLength, 0) != messageLength)
                    errExit("read: insuf bytes", 0);
                if(strncmp(message, "REQ", 3) == 0) {
                    printf("A REQUEST!\n");
                    // new request
                    if(registerRequest(message, messageLength, i, requestList) == -1) {
                        // reject request
                        printf("Rejected\n");
                    }
                    serviceRequest(i, requestList, csock, NULL, 0);
                } else if(strncmp(message, "SRSP", 4) == 0){
                    // response to job
                    printf("received response from client\n");
                    int clientNode;
                    int outputLength;
                    char* outputBuffer = receiveJobResponse(message, messageLength, &outputLength, &clientNode);
                    if(requestList[clientNode].isBroadcast) {
                        char messageBuffer[RESP_MSG_BUFFLEN];
                        int messageLength = sprintf(messageBuffer, "RESP %d ", i);
                        messageLength += sprintf(messageBuffer + messageLength, "%s", outputBuffer);
                        int messageLengthBuff = htonl(messageLength);
                        send(csock[clientNode], &messageLengthBuff, 4, 0);
                        send(csock[clientNode], messageBuffer, messageLength, 0);
                        serviceRequest(clientNode, requestList, csock, NULL, 0);
                    } else
                        serviceRequest(clientNode, requestList, csock, outputBuffer, outputLength);
                } else if(strncmp(message, "TCLS", 4) == 0) {
                    char* token = strtok(message, " ");
                    if(!token || strcmp(token, "TCLS") != 0)
                        continue;
                    token = strtok(NULL, " ");
                    if(!token) continue;
                    int clientNode = atoi(token);
                    if(csock[clientNode] == -1 || requestList[clientNode].state == -2) continue;
                    char messageBuffer[RESP_MSG_BUFFLEN];
                    int messageLength = sprintf(messageBuffer, "RESP %d ", i);
                    messageLength += sprintf(messageBuffer + messageLength, "Shell terminated");
                    int messageLengthBuff = htonl(messageLength);
                    send(csock[clientNode], &messageLengthBuff, 4, 0);
                    send(csock[clientNode], messageBuffer, messageLength, 0);
                    closeRequest(&requestList[clientNode]);
                }
            }
        }
    }
}