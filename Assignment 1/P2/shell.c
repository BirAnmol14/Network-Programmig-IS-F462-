#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define MAX_SHORTCUTS 50
typedef enum tok{CMD1,CMD2,PIPE1,PIPE2,PIPE3,OPT1,OPT2,IPR,OPR,DOPR,BG,COMMA}Token;
typedef struct node{char * lex; Token tok; struct node * next;}Node;
typedef struct ipStream{Node * head;Node * tail;}ipStream;
typedef struct shortCmds{char  ** cmds;}shortCmds;
int terminalPID;
int BGshell = 0;

void die(char *);
void terminal();
void parseCommand(ipStream *,char *);
Node * newNode(char *,Token);
void insert(ipStream * , char * , Token);
ipStream * newIpStream();
void printStream(ipStream *);
char* getTok(Token);
int isBG(ipStream *);
void clearStream(ipStream *);
void handleProc(ipStream *);
void execute(ipStream *);
char* searchPath(char *);
void handleShortcut(ipStream *,shortCmds *);
shortCmds * newShortCmds();
int insertShortcut(shortCmds * ,int,char  *);
int deleteShortcut(shortCmds * ,int,char *);
void clearShortCmds(shortCmds *);
void printShortcuts(shortCmds *);

void shortINT(int);
void sigHandle(int signo, siginfo_t * info ,void * context){
  if(signo == SIGCHLD){
      if(getpid()==terminalPID){
      char *out = malloc(128 * sizeof(char));
      memset(out,'\0',128);
      sprintf(out,"Process %d exited with status %d\n",info->si_pid,info->si_status);
      if(!BGshell) {
        write(fileno(stdout),out,128);
        fflush(stdout);
      }
      free(out);
      if(!BGshell) {
        if(tcsetpgrp(fileno(stdout),getpgid(0))!=0){
          die("failed to change foreground of process");
        }
      }
      kill(terminalPID,SIGCONT);
    }
    wait(NULL);
  }
}
void handle(int signo){
  if(signo==SIGUSR1){

  }else if(signo==SIGINT){
    if(!BGshell) {
      if(tcsetpgrp(fileno(stdout),getppid())!=0){
        die("failed to change foreground of process");
      }
    }
    kill(getppid(),SIGCONT);
    exit(1);
  }
}

int main(int argc, char* argv[]){
  if(argc == 3 && strcmp(argv[1], "--bg") == 0) {
    BGshell = 1;
    if(chdir(argv[2]) < 0)
      exit(1);
  }
  terminal();
}

void die(char * c){
  perror(c);
  exit(1);
}
void terminal(){
  terminalPID = getpid();
  struct sigaction s_act;
  sigemptyset(&(s_act.sa_mask));
  s_act.sa_flags = SA_SIGINFO|SA_RESTART;
  s_act.sa_sigaction = sigHandle;
  sigaction(SIGCHLD,&s_act,NULL);
  shortCmds * sc = newShortCmds();
  while(1){
    if(!BGshell) {
      char * op = malloc(128*sizeof(char));
      memset(op,'\0',128);
      sprintf(op,"/proc/%d/stat",terminalPID);
      int proc = open(op,O_RDONLY);
      int spc=2;
      int jr;
      char x;
      while((jr=read(proc,&x,1))){
        if(spc ==0){
          break;
        }
        if(x==' '){
          spc --;
        }
      }
      if(jr<0)
        die("Process stat read error (terminal)");
      memset(op,'\0',128);
      sprintf(op,"#####################\nTerminal pid: %d(group: %d)\tStatus: %c\n",getpid(),getpgid(0),x);
      write(fileno(stdout),op,strlen(op));
      free(op);

      char * usr = getenv("USER");
      write(fileno(stdout),usr,strlen(usr));
      write(fileno(stdout),"@",1);
      //char * name = getenv("NAME");
      //write(fileno(stdout),name,strlen(name));
      char * cwd = malloc(512 * sizeof(char));
      memset(cwd,'\0',512);
      getcwd(cwd,512);
      //write(fileno(stdout),":",1);
      write(fileno(stdout),cwd,strlen(cwd));
      free(cwd);
      write(fileno(stdout),"$ ",2);
    }
    int n;
    char *readBuf = malloc(BUFSIZ * sizeof(char));
    memset(readBuf,'\0',BUFSIZ);
    long cmdLength = 0;
    char cmdl[7];
    while(1) {
      n = read(fileno(stdin), cmdl, 6);
      if(n == 0) {
        pause();
        continue;
      }
      if(n < 0)
        break;
      // fprintf(stderr, "READ1: %s\n", readBuf);
      cmdl[6] = 0;
      // fprintf(stderr, "%s\n", cmdl);
      cmdLength = strtol(cmdl, NULL, 10);
      // fprintf(stderr, "commandLength: %d\n", cmdLength);
      n = read(fileno(stdin),readBuf,cmdLength);
      // fprintf(stderr, "READ2: %s\n", readBuf);
      if(n > 0)
        break;
    }
   
    if(n<0){
      die("terminal read error");
    }
    readBuf[n+1]='\0';
    ipStream * st = newIpStream();
    parseCommand(st,readBuf);
    int cdFlag =0;
    int scFlag=0;
    Node * p = st->head;
    while(p!=NULL){
      if(p->tok==CMD1 && strcmp(p->lex,"cd")==0){
        cdFlag = 1;
        if(p->next && p->next->lex && p->next->tok == CMD2){
          if(chdir(p->next->lex)<0){
            perror("cd failed");
          }
          write(fileno(stdout), "\n", 1);
          // write(fileno(stdout), p->next->lex, strlen(p->next->lex));
          break;
        }
      }
      p=p->next;
    }
    p=st->head;
    while(p!=NULL){
      if(p->tok==CMD1 && strcmp(p->lex,"sc")==0){
        scFlag = 1;
        break;
      }
      p=p->next;
    }
    if(!cdFlag){
        if(!scFlag){
          handleProc(st);
        }else{
          write(fileno(stdout),"Shortcut Mode Enable\n",strlen("Shortcut Mode Enable\n"));
          handleShortcut(st,sc);
        }
    }
    clearStream(st);
    free(readBuf);
  }
  clearShortCmds(sc);
}
void parseCommand(ipStream * st,char * cmd){
  int state = 0;
  int pos=0;
  int start = 1;
  int len = strlen(cmd);
  char * lex = malloc(BUFSIZ * sizeof(char));
  int i=0;
  while(1){
    switch(state){
      case 0: memset(lex,'\0',BUFSIZ);
              i=0;
              if(pos == len){
                return;
              }
              if(cmd[pos]==' '||cmd[pos]=='\t'){
                  pos++;break;
              }
              if(cmd[pos]=='\n'){
                return;
              }
              if(isalpha(cmd[pos])||isdigit(cmd[pos])||cmd[pos]=='.'||cmd[pos]=='/'){
                  state = 11;
                  lex[i++] = cmd[pos];
                  pos++; break;
              }
              if(cmd[pos] == '^'){
                  state = 1;
                  lex[i++] = cmd[pos];
                  pos++; break;
              }
              if(cmd[pos] == '-'){
                  state = 3;
                  lex[i++] = cmd[pos];
                  pos++; break;
              }
              if(cmd[pos] == '&'){
                  state = 5;
                  lex[i++] = cmd[pos];
                  pos++; break;
              }
              if(cmd[pos] == '|'){
                  state = 6;
                  lex[i++] = cmd[pos];
                  pos++; break;
              }
              if(cmd[pos] == '<'){
                  state = 13;
                  lex[i++] = cmd[pos];
                  pos++; break;
              }
              if(cmd[pos] == '>'){
                  state = 14;
                  lex[i++] = cmd[pos];
                  pos++; break;
              }
              if(cmd[pos]==','){
                state = 17;
                lex[i++] = cmd[pos];
                pos ++;
                break;
              }

      case 1: if(pos == len){
                return;
              }
              if(cmd[pos]!=' ' && cmd[pos] != '\t' && cmd[pos]!='\n'){
                  lex[i++] = cmd[pos++];
                  state = 1; break;
              }
              state=2;
              break;
      case 2: lex[i]='\0';
              insert(st,lex,OPT1);
              state = 0;break;
      case 3: if(pos == len){
                return;
              }
              if(cmd[pos]!=' ' && cmd[pos] != '\t' && cmd[pos]!='\n'){
                  lex[i++] = cmd[pos++];
                  state = 3; break;
              }
              state = 4;
              break;
      case 4: lex[i]='\0';
              insert(st,lex,OPT2);
              state = 0;break;
      case 5: lex[i]='\0';
              insert(st,lex,BG);
              state = 0;
              return;
      case 6: if(pos == len){
                return;
              }
              if(cmd[pos] == '|'){
                  lex[i++] = cmd[pos++];
                  state = 8;
                  break;
              }
              state = 7;
              break;
      case 7: start =1;
              lex[i] = '\0';
              insert(st,lex,PIPE1);
              state = 0;
              break ;
      case 8: if(pos == len){
                  return;
                }
              if(cmd[pos] == '|'){
                lex[i++] = cmd[pos++];
                state = 9;
                break;
              }
              state = 10;
              break;
      case 9: start = 1;
              lex[i] = '\0';
              insert(st,lex,PIPE3);
              state = 0;
              break ;
      case 10: start = 1;
              lex[i] = '\0';
              insert(st,lex,PIPE2);
              state = 0;
              break ;
      case 11:if(pos == len){
                  return;
              }
              if(isalpha(cmd[pos])||isdigit(cmd[pos])||cmd[pos]=='.'||cmd[pos]=='/'){
                  state = 11;
                  lex[i++] = cmd[pos];
                  pos++; break;
              }
              state =12;
              break;
      case 12: lex[i] = '\0';
              if(start == 1){
                insert(st,lex,CMD1);
                start = 0;
              }
              else{
                insert(st,lex,CMD2);
              }
              state = 0;
              break ;
      case 13 : lex[i] = '\0';
                insert(st,lex,IPR);
                state = 0;
                break ;
      case 14: if(pos == len){
                  return;
              }
              if(cmd[pos] == '>'){
                  state = 16;
                  lex[i++] = cmd[pos];
                  pos++; break;
              }
              state = 15;
              break;
      case 15:  lex[i] = '\0';
                insert(st,lex,OPR);
                state = 0;
                break ;

      case 16: lex[i] = '\0';
                insert(st,lex,DOPR);
                state = 0;
                break ;
      case 17: lex[i]='\0';
                start = 1;
                insert(st,lex,COMMA);
                state =0;
                break;
    }
  }
}

Node * newNode(char * lex,Token tk){
  Node * t = malloc(1 * sizeof(Node));
  t->lex = malloc((strlen(lex)+1)* sizeof(char));
  strcpy(t->lex,lex);
  t->tok = tk;
  t->next = NULL;
  return t;
}
void insert(ipStream * s, char * c, Token t){
  if(s->head == NULL){
    s->head = newNode(c,t);
    s->tail = s->head;
    return;
  }
  s->tail->next = newNode(c,t);
  s->tail=s->tail->next;
}
ipStream * newIpStream(){
  ipStream * s = malloc(1 * sizeof(ipStream));
  s->head = NULL;
  s->tail = NULL;
  return s;
}
void printStream(ipStream * st){
  Node * t = st->head;
  while(t!=NULL){
    printf("%s (%s)\n",t->lex,getTok(t->tok));
    t=t->next;
  }
}
char* getTok(Token t){
  switch (t) {
    case CMD1: return "CMD1";
    case CMD2: return "CMD2";
    case PIPE1: return "PIPE1";
    case PIPE2: return "PIPE2";
    case PIPE3: return "PIPE3";
    case OPT1: return "OPT1";
    case OPT2: return "OPT2";
    case IPR:return "IPR";
    case OPR: return "OPR";
    case DOPR: return "DOPR";
    case BG:return "BG";
    case COMMA:return "COMMA";
  }
}

int isBG(ipStream * ts){
  if(ts->tail->tok == BG){
    return 1;
  }
  else{
    return 0;
  }
}

void clearStream(ipStream * s){
  Node * t =s->head;
  Node * p = t;
  while(t!=NULL){
    p=t;
    t=t->next;
    free(p->lex);
    free(p);
  }
  free(s);
}

void handleProc(ipStream * st){
  if(st->head ==NULL){
    return;
  }
  int pid = fork();
  if(isBG(st)){
      if(pid == 0){

          if(setpgid(getpid(),getpid())<0){
            die("failed to change group");
          }
          printf("bg [PID: %d in %d]\n", getpid(),getpgid(0));
          kill(terminalPID,SIGUSR1);
          execute(st);
          exit(0);
      }else{
        signal(SIGUSR1,handle);
        pause();
      }
  }else{

    if(pid == 0){
      signal(SIGUSR1,handle);
      signal(SIGINT,handle);

      if(setpgid(getpid(),getpid())<0){
        die("failed to change group");
      }
      kill(getppid(),SIGUSR1);
      pause();
      if(!BGshell)
        printf("fg [PID: %d in %d]\n", getpid(),getpgid(0));
      execute(st);
      if(!BGshell) {
        if(tcsetpgrp(fileno(stdout),getppid())!=0){
              die("failed to change foreground of process");
        }
      }
      exit(0);
    }else{
      signal(SIGUSR1,handle);
      signal(SIGTTOU,SIG_IGN);
      pause();
      if(!BGshell) {
        if(tcsetpgrp(fileno(stdout),pid)!=0){
          die("failed to change foreground of process");
        }
      }
      kill(pid,SIGUSR1);
      int status;
      waitpid(pid,&status,0);
      signal(SIGTTOU,SIG_DFL);
    }
  }
}

void execute(ipStream * st){
  Node * t = st->head;
  int cmd1s = 0;
  while(t!=NULL){
      if(t->tok == CMD1){
        cmd1s++;
      }
      t=t->next;
  }
  t= st->head;
  Node * start = st->head;
  Node * prev=NULL;
  char* ret = NULL;
  int p1[2];
  pipe(p1);
  int p[2];
  pipe(p);
  Node * END=NULL;
  int pipeChain = 0;
  signal(SIGUSR1,handle);
  while(t!=END){

    if(t->tok == CMD1){
      start = t;
      ret = searchPath(t->lex);
      if(!ret){
        cmd1s--;
        write(fileno(stdout),"Command Not Found\n",strlen("Command Not Found\n"));
        break;
      }
    }
    else if(t->tok == PIPE1){
      pipe(p);
      pipeChain = 1;
      if(prev==NULL){
        die("Invalid Pipe operation");
      }
      int pid = fork();
      if(pid == 0){
        //read from p1/stdin
        //write to p
        Node * r = start;
        Node * end = t;
        Node * pr = NULL;
        int x =0;
        int has = 0;
        int fdout = dup(fileno(stdout));
        char * opb = malloc(128*sizeof(char));
        memset(opb,'\0',128);
        sprintf(opb,"PIPE between %d and %d\n",getppid(),getpid());
        write(fileno(stdout),opb,strlen(opb));
        memset(opb,'\0',128);
        sprintf(opb,"%d reads from %d, writes to %d\n",getpid(),p1[0],p[1]);
        write(fileno(stdout),opb,strlen(opb));
        free(opb);
        close(p[0]);
        close(p1[1]);
        if(start != st->head){
          dup2(p1[0],fileno(stdin));
        }else{
          x =1;
        }
        dup2(p[1],fileno(stdout));
        while(r!=end && r->tok!= BG){
          if(r->tok == IPR || r->tok == OPR ||r->tok ==DOPR){
            end = r;
            has = 1;
            break;
          }
          pr=r;
          r=r->next;
        }
        if(has){
          setvbuf(stdout,NULL,_IONBF,0);
          Node * oper = pr ->next;
          while(oper != t){
            if(oper->tok == IPR){
              if(oper->next==NULL || oper->next->tok != CMD2){
                die("I/P redirection(<)");
              }
              int fd = open(oper->next->lex,O_RDONLY,077);
              if(fd<0){
                die("I/P redirection(<)");
              }
              char * out = malloc(128*sizeof(char));
              sprintf(out,"PID %d Redirecting input: old %d to new %d\n",getpid(),x==1?fileno(stdin):p1[0],fd);
              write(fdout,out,strlen(out));
              free(out);
              if(dup2(fd,fileno(stdin))<0){
                die("I/P redirection(<)");
              }
              close(fd);
            }
            if(oper->tok == OPR){
              if(oper->next==NULL || oper->next->tok != CMD2){
                die("O/P redirection(>)");
              }
              int fd = open(oper->next->lex,O_WRONLY|O_CREAT,0777);
              if(fd<0){
                die("O/P redirection(>)");
              }
              char * out = malloc(128*sizeof(char));
              sprintf(out,"PID %d Redirecting output: old %d to new %d\n",getpid(),p[1],fd);
              write(fdout,out,strlen(out));
              free(out);
              if(dup2(fd,fileno(stdout))<0){
                die("O/P redirection(>)");
              };
              close(fd);
            }
            if(oper->tok == DOPR){
              if(oper->next==NULL || oper->next->tok != CMD2){
                die("O/P redirection(>>)");
              }
              int fd = open(oper->next->lex,O_WRONLY|O_CREAT|O_APPEND,0777);
              if(fd<0){
                die("O/P redirection(>>)");
              }
              char * out = malloc(128*sizeof(char));
              sprintf(out,"PID %d Redirecting output(Append mode): old %d to new %d\n",getpid(),p[1],fd);
              write(fdout,out,strlen(out));
              free(out);
              if(dup2(fd,fileno(stdout))<0){
                die("O/P redirection(>>)");
              };
              close(fd);
            }
            oper = oper->next;
          }
        }
        close(p1[0]);
        close(p[1]);
        close(fdout);
        int argcnt = 0;
        r = start;
        while(r!=end){
          argcnt++;
          r=r->next;
        }
        char *args[argcnt+1];
        int i=0;
        r=start;
        while(r!=end){
          args[i++] = r->lex;
          r=r->next;
        }
        args[i]=NULL;
        execv(ret,args);
        exit(1);
      }else{
        close(p[1]);
        close(p1[1]);
        close(p1[0]);
        wait(NULL);
        char c;
        int n;
        //read p and write to p1
        pipe(p1);
        char * opb = malloc(128*sizeof(char));
        memset(opb,'\0',128);
        sprintf(opb,"%d reads from %d, writes to %d\n",getpid(),p[0],p1[1]);
        write(fileno(stdout),opb,strlen(opb));
        free(opb);
        while((n=read(p[0],&c,1))){
            write(p1[1],&c,1);
        }
        close(p[0]);
        if(n<0){
          die("Final Output error");
        }

        cmd1s--;
      }
    }else if(t->tok == PIPE2){
      Node * cptr = t;
      Node * pk = NULL;
      while(cptr!=NULL){
        if(cptr->tok==COMMA){
          break;
        }
        pk = cptr;
        cptr=cptr->next;
      }
      if(cptr == NULL){
        write(fileno(stdout),"Invalid Use of Double Pipe\n",strlen("Invalid Use of Double Pipe\n"));
        exit(1);
      }
      char *buf=NULL;
      if(pipeChain){
        buf = malloc(3*BUFSIZ * sizeof(char));
        int n ;
        memset(buf,'\0',BUFSIZ);
        n=read(p1[0],buf,BUFSIZ-1);
        if(n<0){
          die("PIPE ERR");
        }
        buf[n+1]='\0';
        close(p[0]);
        close(p[1]);
        close(p1[0]);
        close(p1[1]);
      }
      int pid = fork();
      if(buf!=NULL){
      pipe(p);
      pipe(p1);
      write(p1[1],buf,strlen(buf));
      free(buf);
      buf=NULL;
    }
      if(pid == 0)
      { t->tok = PIPE1;
        memset(t->lex,'\0',strlen(t->lex));
        strcpy(t->lex,"|");
        t=prev;
        END = cptr;
      }
      else{
        waitpid(pid,NULL,0);
        t->tok = PIPE1;
        memset(t->lex,'\0',strlen(t->lex));
        strcpy(t->lex,"|");
        t->next = cptr ->next;
        t=prev;
      }
    }
    else if(t->tok == PIPE3){
      Node * cptr = t;
      Node * pk = NULL;
      while(cptr!=NULL){
        if(cptr->tok==COMMA){
          break;
        }
        pk = cptr;
        cptr=cptr->next;
      }
      if(cptr == NULL){
        write(fileno(stdout),"Invalid Use of Triple Pipe\n",strlen("Invalid Use of Triple Pipe\n"));
        exit(1);
      }
      Node * cptr1 = cptr->next;
      Node * pk1=NULL;
      while(cptr1!=NULL){
        if(cptr1->tok==COMMA){
          break;
        }
        pk1 = cptr1;
        cptr1=cptr1->next;
      }
      if(cptr1 == NULL){
        write(fileno(stdout),"Invalid Use of Triple Pipe\n",strlen("Invalid Use of Triple Pipe\n"));
        exit(1);
      }
      char *buf=NULL;
      if(pipeChain){
        buf = malloc(3*BUFSIZ * sizeof(char));
        int n ;
        memset(buf,'\0',BUFSIZ);
        n=read(p1[0],buf,BUFSIZ-1);
        if(n<0){
          die("PIPE ERR");
        }
        buf[n+1]='\0';
        close(p[0]);
        close(p[1]);
        close(p1[0]);
        close(p1[1]);
      }
      int pid = fork();
      if(buf!=NULL){
      pipe(p);
      pipe(p1);
      write(p1[1],buf,strlen(buf));
      free(buf);
      buf=NULL;
    }
      if(pid == 0)
      { t->tok = PIPE1;
        memset(t->lex,'\0',strlen(t->lex));
        strcpy(t->lex,"|");
        t=prev;
        END = cptr;
      }
      else{
        waitpid(pid,NULL,0);
        t->tok = PIPE2;
        memset(t->lex,'\0',strlen(t->lex));
        strcpy(t->lex,"||");
        t->next = cptr->next;
        t=prev;
      }
    }
    prev = t;
    t=t->next;
  }
  if(cmd1s != 0 && !pipeChain){
    int pid = fork();
    if(pid == 0){
      int argcnt = 0;
      Node * r = start;
      Node * end = END;
      Node * pr = NULL;
      int has = 0;
      while(r!=end && r->tok!= BG){
        if(r->tok == IPR || r->tok == OPR ||r->tok ==DOPR){
          end = r;
          has = 1;
          break;
        }
        pr=r;
        r=r->next;
      }
      if(has){
        setvbuf(stdout,NULL,_IONBF,0);
        Node * oper = pr ->next;
        int fdout = dup(fileno(stdout));
        while(oper != NULL){
          if(oper->tok == IPR){
            if(oper->next==NULL || oper->next->tok != CMD2){
              die("I/P redirection(<)");
            }
            int fd = open(oper->next->lex,O_RDONLY,077);
            if(fd<0){
              die("I/P redirection(<)");
            }
            char * out = malloc(128*sizeof(char));
            sprintf(out,"PID %d Redirecting input: old %d to new %d\n",getpid(),fileno(stdin),fd);
            write(fdout,out,strlen(out));
            free(out);
            if(dup2(fd,fileno(stdin))<0){
              die("I/P redirection(<)");
            }
            //close(fd);
          }
          if(oper->tok == OPR){
            if(oper->next==NULL || oper->next->tok != CMD2){
              die("O/P redirection(>)");
            }
            int fd = open(oper->next->lex,O_WRONLY|O_CREAT,0777);
            if(fd<0){
              die("O/P redirection(>)");
            }
            char * out = malloc(128*sizeof(char));
            sprintf(out,"PID %d Redirecting output: old %d to new %d\n",getpid(),fileno(stdout),fd);
            write(fdout,out,strlen(out));
            free(out);
            if(dup2(fd,fileno(stdout))<0){
              die("O/P redirection(>)");
            };
            //close(fd);
          }
          if(oper->tok == DOPR){
            if(oper->next==NULL || oper->next->tok != CMD2){
              die("O/P redirection(>>)");
            }
            int fd = open(oper->next->lex,O_WRONLY|O_CREAT|O_APPEND,0777);
            if(fd<0){
              die("O/P redirection(>>)");
            }
            char * out = malloc(128*sizeof(char));
            sprintf(out,"PID %d Redirecting output(Append mode): old %d to new %d\n",getpid(),fileno(stdout),fd);
            write(fdout,out,strlen(out));
            free(out);
            if(dup2(fd,fileno(stdout))<0){
              die("O/P redirection(>>)");
            };
            //close(fd);
          }
          oper = oper->next;
        }
        close(fdout);
      }
      r=start;
      while(r!=end && r->tok != BG){
        argcnt++;
        r=r->next;
      }
      char *args[argcnt+1];
      int i=0;
      r=start;
      while(r!=end && r->tok != BG){
        args[i++] = r->lex;
        r=r->next;
      }
      args[i]=NULL;
      execv(ret,args);
      exit(1);
    }else{
      waitpid(pid,NULL,0);
    }
  }
  else if(cmd1s != 0 && pipeChain){
    int pid = fork();
    if(pid == 0){
      //read from p1
      //write to stdout/output redirect
      Node * r = start;
      Node * end = END;
      Node * pr = NULL;
      int has = 0;
      close(p[0]);
      close(p1[1]);
      if(start != st->head){
        dup2(p1[0],fileno(stdin));
      }
      while(r!=end && r->tok!= BG){
        if(r->tok == IPR || r->tok == OPR ||r->tok ==DOPR){
          end = r;
          has = 1;
          break;
        }
        pr=r;
        r=r->next;
      }
      if(has){
        int fdout = dup(fileno(stdout));
        setvbuf(stdout,NULL,_IONBF,0);
        Node * oper = pr ->next;
        while(oper != NULL){
          if(oper->tok == OPR){
            if(oper->next==NULL || oper->next->tok != CMD2){
              die("O/P redirection(>)");
            }
            int fd = open(oper->next->lex,O_WRONLY|O_CREAT,0777);
            if(fd<0){
              die("O/P redirection(>)");
            }
            char * out = malloc(128*sizeof(char));
            sprintf(out,"PID %d Redirecting output: old %d to new %d\n",getpid(),fileno(stdout),fd);
            write(fdout,out,strlen(out));
            free(out);
            if(dup2(fd,fileno(stdout))<0){
              die("O/P redirection(>)");
            };
            //close(fd);
          }
          if(oper->tok == DOPR){
            if(oper->next==NULL || oper->next->tok != CMD2){
              die("O/P redirection(>>)");
            }
            int fd = open(oper->next->lex,O_WRONLY|O_CREAT|O_APPEND,0777);
            if(fd<0){
              die("O/P redirection(>>)");
            }
            char * out = malloc(128*sizeof(char));
            sprintf(out,"PID %d Redirecting output(Append mode): old %d to new %d\n",getpid(),fileno(stdout),fd);
            write(fdout,out,strlen(out));
            free(out);
            if(dup2(fd,fileno(stdout))<0){
              die("O/P redirection(>>)");
            };
            //close(fd);
          }
          oper = oper->next;
        }
        close(fdout);
      }
      close(p1[0]);
      close(p[1]);
      int argcnt = 0;
      r = start;
      while(r!=end && r->tok != BG){
        argcnt++;
        r=r->next;
      }
      char *args[argcnt+1];
      int i=0;
      r=start;
      while(r!=end && r->tok != BG){
        args[i++] = r->lex;
        r=r->next;
      }
      args[i]=NULL;
      execv(ret,args);
      exit(1);
    }else{
        close(p1[0]);
        close(p[0]);
        close(p[1]);
        close(p1[1]);
        wait(NULL);
    }
  }

}

char* searchPath(char * cmd){
  char *path = getenv("PATH");
  char * p1 = malloc((strlen(path)+1)*sizeof(char));
  strcpy(p1,path);
  char * p = strtok(p1,":");
  while(p!=NULL){
    char * test = malloc((strlen(p)+strlen(cmd)+2)*sizeof(char));
    sprintf(test,"%s/%s",p,cmd);
    struct stat s;
    int r =stat(test,&s);
    if(r==0 && s.st_mode && S_IXUSR){
      free(p1);
      return test;
    }
    free(test);
    p=strtok(NULL,":");
  }
  return NULL;
}
int end=0;
int cmd_index=-1;
void handleShortcut(ipStream * st,shortCmds * sc){
  if(st && st->head){
    Node * t = st->head;
    if(t->tok==CMD1 && strcmp(t->lex,"sc")==0 && t->next==NULL){
      if(!t->next){
        while(1){
          struct sigaction s_act;
          sigemptyset(&(s_act.sa_mask));
          s_act.sa_flags = 0;
          s_act.sa_handler = shortINT;
          sigaction(SIGINT,&s_act,NULL);
          char * op = malloc(128*sizeof(char));
          memset(op,'\0',128);
          sprintf(op,"/proc/%d/stat",terminalPID);
          int proc = open(op,O_RDONLY);
          int spc=2;
          int jr;
          char x;
          while((jr=read(proc,&x,1))>0){
            if(spc ==0){
              break;
            }
            if(x==' '){
              spc --;
            }
          }
          if(jr<0){
            die("Process stat read error");
          }
          if(!BGshell) {
            memset(op,'\0',128);
            sprintf(op,"#####################\nTerminal pid: %d(group: %d)\tStatus: %c\n",getpid(),getpgid(0),x);
            write(fileno(stdout),op,strlen(op));
          }
          free(op);
          if(!BGshell) {
            write(fileno(stdout),"> ",2);
            char * usr = getenv("USER");
            write(fileno(stdout),usr,strlen(usr));
            write(fileno(stdout),"@",1);
            //char * name = getenv("NAME");
            //write(fileno(stdout),name,strlen(name));
            char * cwd = malloc(512 * sizeof(char));
            memset(cwd,'\0',512);
            getcwd(cwd,512);
            //write(fileno(stdout),":",1);
            write(fileno(stdout),cwd,strlen(cwd));
            free(cwd);
            write(fileno(stdout),"$ ",2);
          }
          int n;
          char *readBuf = malloc(BUFSIZ * sizeof(char));
          memset(readBuf,'\0',BUFSIZ);
          n = read(fileno(stdin),readBuf,BUFSIZ-1);
          if(n<0){
            if(end)break;
            if(cmd_index>=0){
              if(sc->cmds[cmd_index]==NULL){
                write(fileno(stdout),"No command present at this index\n",strlen("No command present at this index\n"));
                continue;
              }
              if(strcmp(sc->cmds[cmd_index],"cd")==0){
                write(fileno(stdout),"Can't execute cd\n",strlen("Can't execute cd\n"));
              }
              else if(strcmp(sc->cmds[cmd_index],"sc")==0){
                  break;
              }else{
                int pid = fork();
                if(pid == 0){
                  execlp(sc->cmds[cmd_index],sc->cmds[cmd_index],NULL);
                }else{
                  waitpid(pid,NULL,0);
                }
              }
            }
              continue;
          }
          readBuf[n+1]='\0';
          ipStream * st = newIpStream();
          parseCommand(st,readBuf);
          int cdFlag =0;
          int scFlag=0;
          Node * p = st->head;
          while(p!=NULL){
            if(p->tok==CMD1 && strcmp(p->lex,"cd")==0){
              cdFlag = 1;
              if(p->next && p->next->lex && p->next->tok == CMD2){
                if(chdir(p->next->lex)<0){
                  perror("cd failed");
                }
                write(fileno(stdout), "\n", 1);
                break;
              }
            }
            p=p->next;
          }
          p=st->head;
          while(p!=NULL){
            if(p->tok==CMD1 && strcmp(p->lex,"sc")==0){
              scFlag = 1;
              break;
            }
            p=p->next;
          }

          if(!cdFlag){
              if(!scFlag){
                handleProc(st);
              }else{
                break;
              }
          }
          clearStream(st);
          free(readBuf);
        }
        write(fileno(stdout),"Shortcut Mode Disabled\n",strlen("Shortcut Mode Disabled\n"));
        signal(SIGINT,SIG_DFL);
      }else{
        if(t->next->tok == OPT2 ){
          if(strcmp(t->next->lex,"-d")==0){
            //delete
            Node * p1 = t->next;
            if(p1->next==NULL || p1->next->lex==NULL ||p1->next->tok!=CMD2 ||p1->next->next == NULL || p1->next->next->lex == NULL||p1->next->next->tok != CMD2 || p1->next->next->next != NULL){
                write(fileno(stdout),"Invalid Operation\n",strlen("Invalid Operation\n"));
            }else{
              Node * index = p1->next;
              Node * cmd = index->next;
              int ret=deleteShortcut(sc,atoi(index->lex),cmd->lex);
              if(ret==-1){
                  write(fileno(stdout),"Failed to delete\n",strlen("Failed to delete\n"));
              }else{
                  write(fileno(stdout),"Deleted Successfully\n",strlen("Deleted Successfully\n"));
              }
            }
          }else if(strcmp(t->next->lex,"-i")==0){
              //insert
              Node * p1 = t->next;
              if(p1->next==NULL || p1->next->lex==NULL ||p1->next->tok!=CMD2 ||p1->next->next == NULL || p1->next->next->lex == NULL||p1->next->next->tok != CMD2 || p1->next->next->next != NULL){
                  write(fileno(stdout),"Invalid Operation\n",strlen("Invalid Operation\n"));
              }else{
                Node * index = p1->next;
                Node * cmd = index->next;
                int ret=insertShortcut(sc,atoi(index->lex),cmd->lex);
                if(ret==-1){
                    write(fileno(stdout),"Failed to insert\n",strlen("Failed to insert\n"));
                }else{
                    write(fileno(stdout),"Inserted Successfully\n",strlen("Inserted Successfully\n"));
                }
              }
            }else{
              write(fileno(stdout),"Invalid Option\n",strlen("Invalid Option\n"));
            }
        }else{
          write(fileno(stdout),"Invalid Option\n",strlen("Invalid Option\n"));
        }
        write(fileno(stdout),"Shortcut Mode Disabled\n",strlen("Shortcut Mode Disabled\n"));
      }
    }
  }
}
void shortINT(int signo){
  if(signo == SIGINT){
    write(fileno(stdout),"\nEnter Command Number(-1 to end shortcut mode): ",strlen("\nEnter Command Number(-1 to end shortcut mode): "));
    char * c = malloc(25 * sizeof(char));
    memset(c,'\0',25);
    int n=read(fileno(stdin),c,24);
    if(n<0){
      die("read");
    }
    int cmdi = atoi(c);
    if(cmdi <-1 || cmdi >=MAX_SHORTCUTS){
        write(fileno(stdout),"Invalid Index\n",strlen("Invalid Index\n"));
        cmd_index=-1;
    }else if(cmdi!=-1){
      cmd_index = cmdi;
    }
    else if(cmdi == -1){
        signal(SIGINT,SIG_DFL);
          end = 1;
    }
  }
}
shortCmds * newShortCmds(){
  shortCmds * s = malloc(1*sizeof(shortCmds));
  s->cmds = malloc(MAX_SHORTCUTS * sizeof(char *));
  for(int i=0;i<MAX_SHORTCUTS;i++){
    s->cmds[i] = NULL;
  }
  return s;
}
int insertShortcut(shortCmds * s,int index,char * cmd){
  if(s==NULL || index<0||index>=MAX_SHORTCUTS||s->cmds ==NULL||s->cmds[index]!=NULL){
    return -1;
  }
  s->cmds[index] = malloc((strlen(cmd)+1)*sizeof(char));
  strcpy(s->cmds[index],cmd);
  return 1;
}
int deleteShortcut(shortCmds * s,int index,char * cmd){
  if(s==NULL || index<0||index>=MAX_SHORTCUTS||s->cmds ==NULL || s->cmds[index] == NULL){
    return -1;
  }
  if(strcmp(s->cmds[index],cmd)==0){
    free(s->cmds[index]);
    s->cmds[index]=NULL;
    return 1;
  }
  return -1;
}

void clearShortCmds(shortCmds * s){
  if(s!=NULL){
    for(int i=0;i<MAX_SHORTCUTS;i++){
      if(s->cmds[i]){
        free(s->cmds[i]);
        s->cmds[i]=NULL;
      }
    }
  }
  free(s);
}

void printShortcuts(shortCmds *st){
  if(!st || !st->cmds)
  {return;}
  for(int i=0;i<MAX_SHORTCUTS;i++){
    if(st->cmds[i]){
      char * out = malloc(128 * sizeof(char));
      sprintf(out,"%d: %s\n",i,st->cmds[i]);
      write(fileno(stdout),out,strlen(out));
      free(out);
    }
  }
}
