#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#define MAX_NODES 256
#define INPUT_BUFF_LEN 256
#define OUTPUT_BUFF_LEN 1024
#define REQ_MSG_BUFFLEN 256
#define SERVRESP_MSG_BUFFLEN 2048

typedef struct shellInst {
    int pid;
    char user[33];
    int pipein;
    int pipeout;
    char fifoName[11];
} shellInst;

typedef struct Job {
    int node;   
    int commandLength;
    char* command;
} Job;

shellInst shellList[MAX_NODES+1];
int numShells = 0;

int sock = -1;
char* serverIP = "127.0.0.1";
int serverPort = 31415;

void errExit(char* msg, int printErrNo) {
    if(printErrNo)
        perror(msg);
    else
        fprintf(stderr, "%s\n", msg);
    fprintf(stderr, "Exiting...\n");
    exit(EXIT_FAILURE);
}

void sigchldHandle(int signo) {
    int terminalpid;
    while((terminalpid = waitpid(-1, NULL, WNOHANG)) > 0) {
        if(shellList[0].pid == terminalpid) {
            printf("Local Shell terminated.\n");
        }
        for(int i = 1; i <= MAX_NODES; i++) {
            if(shellList[i].pid == terminalpid) {
                char message[10];
                int messageLength = sprintf(message, "TCLS %d", i);
                int messageLengthBuff = htonl(messageLength);
                send(sock, &messageLengthBuff, 4, 0);
                send(sock, message, messageLength, 0);
                shellList[i].pid = -1;
                break;
            }
        }
        // if no i matches, terminal closed by CLOS
        // no action required
    }
}

void initializeClient(void) {
    // create socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1) 
        errExit("socket", 1);
   
    // connect to server
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr.s_addr));
    serverAddr.sin_port = htons(serverPort);
    if(connect(sock, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) == -1)
        errExit("connect", 1);
    
    for(int i = 0; i < MAX_NODES; i++)
        shellList[i].pid = -1;

    signal(SIGCHLD, sigchldHandle);
}

int createShellInst(int clientNode, char user[33]) {
    char fifoName[11];
    int nl = sprintf(fifoName, "sh%d.fifo", clientNode);
    fifoName[nl] = 0;
    mkfifo(fifoName, 0777);
    int pipeOut[2];
    pipe(pipeOut);
    // create shell process
    int childPid = fork();
    if(childPid == -1) {
        errExit("fork", 1);
    } else if(childPid == 0) {
        // child process - shell
        // close unused pipe ends
        close(pipeOut[0]);
        // replace stdin with pipeIn[0]
        close(0);
        int wfd = open(fifoName, O_RDONLY);
        if(wfd != 0) {
            dup2(wfd, 0);
        }
        // replace stdout with pipeOut[1]
        close(1);
        dup(pipeOut[1]);
        // execl("shell2", "shell2", (char*) NULL);
        char userpath[40];
        sprintf(userpath, "/home/%s/", user);
        execl("shell3", "shell3", "--bg", userpath, (char*) NULL);
        errExit("execl", 1);
    } else {
        // parent process - client
        close(pipeOut[1]);
    }
    shellInst* si = &shellList[clientNode];
    si -> pid = childPid;
    strcpy(si -> user, user);
    si -> pipein = open(fifoName, O_WRONLY);
    si -> pipeout = pipeOut[0];
    strcpy(si -> fifoName, fifoName);
    numShells++;
    return 0;
}

int getNumJobs(char* cmdInput) {
    int numJobs = 1;
    while(cmdInput = strchr(cmdInput, '|')) {
        numJobs++;
        cmdInput++;
    }
    return numJobs;
}

char* trimString(char* str, int* len) {
    while(*len > 0 && isspace(str[0])) {
        str++;
        (*len)--;
    }
    while(*len > 0 && isspace(str[*len - 1])) {
        (*len)--;
    }
    return str;
}

Job parseCommand(char* cmdInput, int len) {
    cmdInput = trimString(cmdInput, &len);
    Job j;
    if(strncmp(cmdInput, "nodes", len) == 0) {
        j.node = -1;
    } else if(cmdInput[0] == 'n') {
        char* nextCmdInput;
        long node_val = strtol(cmdInput + 1, &nextCmdInput, 10);
        if((nextCmdInput == cmdInput + 1) || (*nextCmdInput != '.')) {
            if(cmdInput[1] == '*' && cmdInput[2] == '.') {
                // broadcast case
                j.node = -1;
                len -= 3;
                cmdInput += 3;
            } else {
                j.node = 0;
            }
        } else {
            j.node = (int) node_val; // should check for overflow
            len -= (nextCmdInput + 1 - cmdInput);
            cmdInput = nextCmdInput + 1;
        }
    } else {
        j.node = 0;
    }
    cmdInput = trimString(cmdInput, &len);
    j.commandLength = len;
    j.command = malloc((len + 1) * sizeof(char));
    strncpy(j.command, cmdInput, len);
    j.command[len] = 0;
    return j;
}

int parseInput(char* cmdInput, Job** jobs) {
    int numJobs = getNumJobs(cmdInput);
    int inputLen = strlen(cmdInput);
    *jobs = malloc(numJobs * sizeof(Job));
    for(int i = 0; i < numJobs; i++) {
        int len = 0;
        while(len < inputLen && cmdInput[len] != '|')
            len++;
        (*jobs)[i] = parseCommand(cmdInput, len);
        cmdInput += len + 1;
        inputLen -= len + 1;  
    }
    return numJobs;
}

void printJobList(Job* jobs, int numJobs) {
    printf("numJobs: %d\n", numJobs);
    for(int i = 0; i < numJobs; i++) {
        printf("JOB %d\nnode: %d\ncommand: %s\nlen:%d\n", i+1, jobs[i].node, jobs[i].command, jobs[i].commandLength);
    }
}

int isJobListLocal(Job* jobs, int numJobs) {
    for(int i = 0; i < numJobs; i++)
        if(jobs[i].node != 0)
            return 0;
    return 1;
}

void executeLocalCommand(char* cmd, int cmdLength) {
    int i;
    char* localUser = getenv("USER");
    shellInst* si = &shellList[0];
    if(si -> pid == -1)
        createShellInst(0, localUser);
    cmd[cmdLength] = '\n';
    if(write(si -> pipein, cmd, cmdLength) == -1)
        errExit("write", 1);
}

void sendJobList(Job* jobs, int numJobs) {
    char messageBuffer[REQ_MSG_BUFFLEN];
    int messageLength = sprintf(messageBuffer, "REQ %d %s\n", numJobs, getenv("USER"));
    for(int i = 0; i < numJobs; i++) {
        messageLength += sprintf(messageBuffer + messageLength, "%d %d\n%s\n", 
            jobs[i].node, jobs[i].commandLength, jobs[i].command);
    }
    // messageLength += sprintf(messageBuffer + messageLength, "\n");
    printf("ML: %d\n", messageLength);
    int messageLengthBuff = htonl(messageLength);
    send(sock, &messageLengthBuff, 4, 0);
    send(sock, messageBuffer, messageLength, 0);
}

int writeServerReqToShell(char* message, int messageLength) {
    char* token;
    token = strtok(message, " ");
    if(token == NULL || strcmp(token, "SREQ") != 0)
        return -1;
    token = strtok(NULL, " ");
    if(!token) return -1;
    int clientNode = atoi(token);
    char user[33];
    token = strtok(NULL, " ");
    if(!token) return -1;
    strncpy(user, token, strlen(token));
    user[strlen(token)] = 0;
    char* cmdInput = token + strlen(token) + 1;
    int cmdLength = messageLength - (int)(cmdInput - message);
    // append EOT
    cmdInput[cmdLength] = '\n';
    cmdInput[cmdLength + 1] = '\x04';
    cmdLength += 2;

    shellInst* si = &shellList[clientNode];
    if(si -> pid == -1)
        createShellInst(clientNode, user);

    if(si -> pipein == -1)
        si -> pipein = open(si -> fifoName, O_WRONLY);
    if(write(si->pipein, cmdInput, cmdLength) == -1)
        errExit("write (to pipe)", 1);
    close(si -> pipein);    
    si -> pipein = -1;
    // sleep(1);
    // si -> pipein = open(si -> fifoName, O_WRONLY);
}

int writeServerRespOut(char* message, int messageLength) {
    char* token;
    token = strtok(message, " ");
    if(token == NULL || strcmp(token, "RESP") != 0)
        return -1;
    token = strtok(NULL, " ");
    if(!token) return -1;
    int remoteNode = atoi(token);
    if(remoteNode == 0)
        printf("[Server]:\n");
    else
        printf("[n%d]:\n", remoteNode);
    message[messageLength];
    char* output = token + strlen(token) + 1;
    printf("%s\n", output);
}

int writeOutputToServer(char* outputBuffer, int outputLength, int clientNode) {
    char messageBuffer[SERVRESP_MSG_BUFFLEN];
    int messageLength = sprintf(messageBuffer, "SRSP %d ", clientNode);
    messageLength += sprintf(messageBuffer + messageLength, "%s", outputBuffer);
    int messageLengthBuff = htonl(messageLength);
    send(sock, &messageLengthBuff, 4, 0);
    send(sock, messageBuffer, messageLength, 0);
}

int main(void) {
    initializeClient();
    printf("$ ");
    fflush(stdout);

    char cmdInput[INPUT_BUFF_LEN];
    Job* jobs;

    fd_set readSet;
    int maxfd;
    int stdinlock = 0;
    while(1) {
        maxfd = 0;
        FD_ZERO(&readSet);
        if(stdinlock == 0)
            FD_SET(STDIN_FILENO, &readSet);
        FD_SET(sock, &readSet);
        maxfd = (STDIN_FILENO > sock) ? STDIN_FILENO : sock;
        for(int i = 0; i <= MAX_NODES; i++) {
            if(shellList[i].pid > 0) {
                FD_SET(shellList[i].pipeout, &readSet);
                if(maxfd < shellList[i].pipeout)
                    maxfd = shellList[i].pipeout;
            }
        }
        int sr = select(maxfd + 1, &readSet, NULL, NULL, NULL);
        if(sr == -1)
            errExit("select", 1);
        if(sr > 0 && FD_ISSET(STDIN_FILENO, &readSet)) {
            // stdin readable
            fgets(cmdInput, INPUT_BUFF_LEN, stdin);
            if(strcmp(cmdInput, "\n") == 0)
                continue;
            int numJobs = parseInput(cmdInput, &jobs);
            printJobList(jobs, numJobs);
            if(isJobListLocal(jobs, numJobs)) {
                // execute local job
                executeLocalCommand(cmdInput, strlen(cmdInput));
                printf("$ ");
                fflush(stdout);
            } else {
                sendJobList(jobs, numJobs);
                stdinlock = 1;
            }
            free(jobs);
            sr--;
        }
        if(sr > 0 && FD_ISSET(sock, &readSet)) {
            // socket readable
            int messageLength;
            int bytesRecvd = recv(sock, &messageLength, 4, 0);
            if(bytesRecvd == 0) {
                // server disconnected
                errExit("Server: Connection terminated", 0);
            }
            messageLength = ntohl(messageLength);
            printf("ML: %d\n", messageLength);
            char message[messageLength+1];
            if(recv(sock, message, messageLength, 0) != messageLength)
                errExit("read: insuf bytes", 0);
            message[messageLength] = 0;
            if(strncmp(message, "SREQ", 4) == 0) {
                // new request from server
                printf("Received new request from server\n");
                writeServerReqToShell(message, messageLength);
            } else if(strncmp(message, "RESP", 4) == 0){
                // response from server
                printf("received final response\n");
                writeServerRespOut(message, messageLength);
                stdinlock = 0;
                printf("$ ");
                fflush(stdout);
            } else if(strncmp(message, "CLOS", 4) == 0){
                // close notification
                char* token = strtok(message, " ");
                if(!token || strcmp(token, "CLOS") != 0)
                    continue;
                token = strtok(NULL, " ");
                if(!token) continue;
                int node = atoi(token);
                if(node < MAX_NODES && shellList[node].pid > 0) {
                    int pid = shellList[node].pid;
                    shellList[node].pid = -1;
                    close(shellList[node].pipein);
                    close(shellList[node].pipeout);
                    kill(pid, SIGINT);
                }
            } else {
                printf("UNKNOWN MSG\n");
                char code[5];
                strncpy(code, message, 4);
                code[4] = 0;
                printf("%s\n", code);
            }
            sr--;
        }
        for(int i = 0; sr > 0 && i <= MAX_NODES; i++) {
            if(shellList[i].pid == -1) continue;
            if(FD_ISSET(shellList[i].pipeout, &readSet)) {
                // pipeout readable for shell[i]
                // printf("pipe output available\n");
                char outputBuffer[OUTPUT_BUFF_LEN];
                int bytesRead = read(shellList[i].pipeout, outputBuffer, OUTPUT_BUFF_LEN - 1);
                if(bytesRead == -1)
                    errExit("read", 1);
                outputBuffer[bytesRead] = 0;
                if(i == 0)
                    printf("%s", outputBuffer);
                else
                    writeOutputToServer(outputBuffer, bytesRead, i);
            }
        }
    }

}