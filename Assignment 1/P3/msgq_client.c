#include "msgq.h"
int logStatus = 0;
int cli;
int serv;
char * myPath;
int getServer();
int getMyQueue();
int Options();
void closeMyQueue(int );
void Login();
void Logout();
void pvtMsg();
void List();
void JoinGrp();
void CreateGrp();
void msgToGrp();
void printMessage(Message *);
void readMsg();
void die(char * s){
  perror(s);
  exit(1);
}

int main(){
  serv = getServer();
  cli = getMyQueue();
  while(1){
        int ret = Options();
        if (ret==-1){
          break;
        }
        if(ret == 0){
          puts("Invalid Option");
        }
  }
  Logout();
  closeMyQueue(cli);
}

int getServer(){
  int serv = msgget(ftok(ServerQ,ProjID),IPC_CREAT|IPC_EXCL|S_IWUSR);
  if(errno == EEXIST){
    serv = msgget(ftok(ServerQ,ProjID),IPC_CREAT|S_IWUSR);
  }else if(serv>0){
    msgctl(serv,IPC_RMID,NULL);
    die("Server was not existent");
  }
  if(serv<0){
    die("Unable to get server queue");
  }
  return serv;
}

int getMyQueue(){
  char * fname= malloc(128 * sizeof(char));
  memset(fname,'\0',128);
  sprintf(fname,"cli_%d_%d",getuid(),getpid());
  myPath = malloc(strlen(fname)*sizeof(char));
  strcpy(myPath,fname);

  int file = open(myPath, O_CREAT|O_RDONLY,0666);
  if(file<0){
    die("Error in creating unique id");
  }
  close(file);

  int cli = msgget(ftok(myPath,ProjID),IPC_EXCL|IPC_CREAT|S_IRUSR|S_IWUSR);
  if(cli<0){
    die("Unable to get client queue");
  }
  free(fname);
  return cli;
}

void closeMyQueue(int id){
  int ret = remove(myPath);
  free(myPath);
  if(ret<0){
    die("Unable to remove file");
  }
  int cli = msgctl(id,IPC_RMID,NULL);
  if(cli<0){
    die("Unable to remove client queue");
  }
  return;
}

int Options(){
  puts("************OPTIONS************");
  int opt;
  if(logStatus == 0){
      puts("1. Login");
      puts("2. Exit");
      puts("Enter Option: ");
      scanf("%d",&opt);getchar();
      if(opt == 1){
        Login();return opt;
      }
      else if(opt==2) {
        return -1;
      }
      else{
        return 0;
      }
  }
  else{
    printf("Welcome %s\n",myPath);
    puts("1. Logout");
    puts("2. Exit");
    puts("3. Send a Private Message");
    puts("4. List");
    puts("5. Join a Group");
    puts("6. Send Message to a Group");
    puts("7. Create New Group");
    puts("8. Read New Messages");
    puts("Enter Option: ");
    scanf("%d",&opt);getchar();
    if(opt == 1){
      Logout();logStatus = 0;return opt;
    }
    else if(opt==2) {
      return -1;
    }
    else if(opt==3){
      pvtMsg();
    }
    else if(opt==4){
      List();
    }
    else if(opt==5){
      JoinGrp();
    }
    else if(opt==6){
      msgToGrp();
    }
    else if(opt==7){
      CreateGrp();
    }
    else if(opt==8){
      readMsg();
    }
    else{
      return 0;
    }
  }

  puts("*******************************");
}


void Login(){
  Message m ;
  memset(m.from,'\0',USR_NAME_MAX);
  memset(m.to,'\0',USR_NAME_MAX);
  memset(m.msg,'\0',MSG_MAX);
  m.mtype = 1;
  m.type = LOGIN;
  strcpy(m.from,myPath);
  strcpy(m.to,"system");
  m.group_id = -1;
  strcpy(m.msg,"LOGIN");
  m.auto_delete = FALSE;
  (m.ts).msg_time = time(NULL);
  (m.ts).duration = -1;
  int r =msgsnd(serv,&m,sizeof(m)-sizeof(long),0);
  if(r<0){closeMyQueue(cli);die("msgsnd");}
  Message m1;
  memset(m1.from,'\0',USR_NAME_MAX);
  memset(m1.to,'\0',USR_NAME_MAX);
  memset(m1.msg,'\0',MSG_MAX);
  msgrcv(cli,&m1,sizeof(Message)-sizeof(long),1,0);
  if(m1.type == RESPONSE){
    printMessage(&m1);
    if(strcmp(m1.msg,"OK")==0){
      logStatus = 1;
    }else{
      logStatus = 0;
      printf("%s\n",m1.msg);
    }
  }
  else{
    logStatus = 0;
    printf("Failed to Login\n");
  }
  //read pending messages
  puts("Fetching Pending Messages");
  sleep(1);
  readMsg();
}
void Logout(){
  Message m ;
  memset(m.from,'\0',USR_NAME_MAX);
  memset(m.to,'\0',USR_NAME_MAX);
  memset(m.msg,'\0',MSG_MAX);
  m.mtype = 1;
  m.type = LOGOUT;
  strcpy(m.from,myPath);
  strcpy(m.to,"system");
  m.group_id = -1;
  strcpy(m.msg,"LOGOUT");
  m.auto_delete = FALSE;
  (m.ts).msg_time = time(NULL);
  (m.ts).duration = -1;
  int r =msgsnd(serv,&m,sizeof(m)-sizeof(long),0);
  if(r<0){closeMyQueue(cli);die("msgsnd");}
  Message m1;
  memset(m1.from,'\0',USR_NAME_MAX);
  memset(m1.to,'\0',USR_NAME_MAX);
  memset(m1.msg,'\0',MSG_MAX);
  msgrcv(cli,&m1,sizeof(Message)-sizeof(long),1,0);
  if(m1.type == RESPONSE){
    printMessage(&m1);
    if(strcmp(m1.msg,"OK")==0){
      logStatus = 0;
    }else{
      logStatus = 1;
      printf("%s\n",m1.msg);
    }
  }
  else{
    logStatus = 0;
    printf("Failed to Logout\n");
  }
}
void pvtMsg(){
  Message m ;
  memset(m.from,'\0',USR_NAME_MAX);
  memset(m.to,'\0',USR_NAME_MAX);
  memset(m.msg,'\0',MSG_MAX);
  m.mtype = 2;
  m.type = SND_MSG;
  strcpy(m.from,myPath);
  printf("Enter Other user (uid followed by(after enter) pid)\n");
  int uid ,pid;
  scanf("%d", &uid);getchar();
  scanf("%d",&pid);getchar();
  sprintf(m.to,"cli_%d_%d",uid,pid);
  m.group_id = -1;
  printf("Enter message(<%d characters)\n",MSG_MAX);
  scanf("%[^\n]",m.msg );
  getchar();
  printf("Do you want auto-delete? (1-Yes,0-No)\n");
  int opt ;
  scanf("%d",&opt);getchar();
  if(opt==1){
    m.auto_delete = TRUE;
    (m.ts).msg_time = time(NULL);
    printf("What duration (in secs)?\n");
    int sec ;
    scanf("%d",&sec);getchar();
    (m.ts).duration = sec;
  }else{
    m.auto_delete = FALSE;
    (m.ts).msg_time = time(NULL);
    (m.ts).duration = -1;
  }
  msgsnd(serv,&m,sizeof(Message)-sizeof(long),0);
  Message m1;
  memset(m1.from,'\0',USR_NAME_MAX);
  memset(m1.to,'\0',USR_NAME_MAX);
  memset(m1.msg,'\0',MSG_MAX);
  msgrcv(cli,&m1,sizeof(Message)-sizeof(long),1,0);
  if(m1.type == RESPONSE){
    printMessage(&m1);
    if(strcmp(m1.msg,"OK")==0){
      printf("Message Sent\n");
    }else{
      printf("%s\n",m1.msg);
    }
  }

}
void List(){
  Message m ;
  memset(m.from,'\0',USR_NAME_MAX);
  memset(m.to,'\0',USR_NAME_MAX);
  memset(m.msg,'\0',MSG_MAX);
  m.mtype = 1;
  m.type = LIST;
  strcpy(m.from,myPath);
  strcpy(m.to,"system");
  m.group_id = -1;
  strcpy(m.msg,"LIST");
  m.auto_delete = FALSE;
  (m.ts).msg_time = time(NULL);
  (m.ts).duration = -1;
  int r =msgsnd(serv,&m,sizeof(m)-sizeof(long),0);
  if(r<0){closeMyQueue(cli);die("msgsnd");}
  Message m1;
  memset(m1.from,'\0',USR_NAME_MAX);
  memset(m1.to,'\0',USR_NAME_MAX);
  memset(m1.msg,'\0',MSG_MAX);
  puts("Fetching Results");
  sleep(1);
  while((r= msgrcv(cli,&m1,sizeof(Message)-sizeof(long),1,IPC_NOWAIT))>0){
    if(m1.type == RESPONSE){
      printMessage(&m1);
      memset(m1.msg,'\0',MSG_MAX);
      memset(m1.from,'\0',USR_NAME_MAX);
      memset(m1.to,'\0',USR_NAME_MAX);
    }
  }
}
void JoinGrp(){
    Message m ;
    memset(m.from,'\0',USR_NAME_MAX);
    memset(m.to,'\0',USR_NAME_MAX);
    memset(m.msg,'\0',MSG_MAX);
    m.mtype = 1;
    m.type = JOIN_GRP;
    strcpy(m.from,myPath);
    strcpy(m.to,"system");
    puts("Enter Group Id:");
    int gid;scanf("%d",&gid);getchar();
    m.group_id = gid;
    strcpy(m.msg,"JOIN Group");
    m.auto_delete = FALSE;
    (m.ts).msg_time = time(NULL);
    (m.ts).duration = -1;
    int r =msgsnd(serv,&m,sizeof(m)-sizeof(long),0);
    if(r<0){closeMyQueue(cli);die("msgsnd");}
    Message m1;
    memset(m1.from,'\0',USR_NAME_MAX);
    memset(m1.to,'\0',USR_NAME_MAX);
    memset(m1.msg,'\0',MSG_MAX);
    msgrcv(cli,&m1,sizeof(Message)-sizeof(long),1,0);
    if(m1.type == RESPONSE){
        printMessage(&m1);
        memset(m1.msg,'\0',MSG_MAX);
        memset(m1.from,'\0',USR_NAME_MAX);
        memset(m1.to,'\0',USR_NAME_MAX);
    }
    puts("Fetching Past Messages");
    sleep(1);
    while((r= msgrcv(cli,&m1,sizeof(Message)-sizeof(long),3,IPC_NOWAIT))>0){

        printMessage(&m1);
        memset(m1.msg,'\0',MSG_MAX);
        memset(m1.from,'\0',USR_NAME_MAX);
        memset(m1.to,'\0',USR_NAME_MAX);
      
    }
}
void CreateGrp(){
  Message m ;
  memset(m.from,'\0',USR_NAME_MAX);
  memset(m.to,'\0',USR_NAME_MAX);
  memset(m.msg,'\0',MSG_MAX);
  m.mtype = 1;
  m.type = CREATE_GRP;
  strcpy(m.from,myPath);
  strcpy(m.to,"system");
  m.group_id = -1;
  strcpy(m.msg,"CREATE GROUP");
  m.auto_delete = FALSE;
  (m.ts).msg_time = time(NULL);
  (m.ts).duration = -1;
  int r =msgsnd(serv,&m,sizeof(m)-sizeof(long),0);
  if(r<0){closeMyQueue(cli);die("msgsnd");}
  Message m1;
  memset(m1.from,'\0',USR_NAME_MAX);
  memset(m1.to,'\0',USR_NAME_MAX);
  memset(m1.msg,'\0',MSG_MAX);
  r= msgrcv(cli,&m1,sizeof(Message)-sizeof(long),1,0);
  if(m1.type == RESPONSE){
      printMessage(&m1);
      memset(m1.msg,'\0',MSG_MAX);
      memset(m1.from,'\0',USR_NAME_MAX);
      memset(m1.to,'\0',USR_NAME_MAX);
  }
}
void msgToGrp(){
  Message m ;
  memset(m.from,'\0',USR_NAME_MAX);
  memset(m.to,'\0',USR_NAME_MAX);
  memset(m.msg,'\0',MSG_MAX);
  m.mtype = 2;
  m.type = GRP_MSG;
  strcpy(m.from,myPath);
  strcpy(m.to,"system");
  puts("Enter Group Id:");
  int gid;scanf("%d",&gid);getchar();
  m.group_id = gid;
  printf("Enter message(<%d characters)\n",MSG_MAX);
  scanf("%[^\n]",m.msg);getchar();
  printf("Do you want auto-delete? (1-Yes,0-No)\n");
  int opt ;
  scanf("%d",&opt);getchar();
  if(opt==1){
    m.auto_delete = TRUE;
    (m.ts).msg_time = time(NULL);
    printf("What duration (in secs)?\n");
    int sec ;
    scanf("%d",&sec);getchar();
    (m.ts).duration = sec;
  }else{
    m.auto_delete = FALSE;
    (m.ts).msg_time = time(NULL);
    (m.ts).duration = -1;
  }
  int r =msgsnd(serv,&m,sizeof(m)-sizeof(long),0);
  if(r<0){closeMyQueue(cli);die("msgsnd");}
  Message m1;
  memset(m1.from,'\0',USR_NAME_MAX);
  memset(m1.to,'\0',USR_NAME_MAX);
  memset(m1.msg,'\0',MSG_MAX);
  r= msgrcv(cli,&m1,sizeof(Message)-sizeof(long),1,0);
  if(m1.type == RESPONSE){
    printMessage(&m1);
    if(strcmp(m1.msg,"OK")==0){
      printf("Message Sent\n");
    }else{
      printf("%s\n",m1.msg);
    }
  }
}
void readMsg(){
  Message m;
  memset(m.from,'\0',USR_NAME_MAX);
  memset(m.to,'\0',USR_NAME_MAX);
  memset(m.msg,'\0',MSG_MAX);
  int r;
  while((r=msgrcv(cli,&m,sizeof(Message)-sizeof(long),2,IPC_NOWAIT))>=0)
  {
    printMessage(&m);
    memset(m.from,'\0',USR_NAME_MAX);
    memset(m.to,'\0',USR_NAME_MAX);
    memset(m.msg,'\0',MSG_MAX);
  }
  if(r<0){
    return;
  }
}
void printMessage(Message * m){
  if(m->type==RESPONSE){
    printf("%s\n",m->msg);
  }else if(m->type == SND_MSG){
    printf("%s: %s\n",m->from,m->msg);
  }else if(m->type == GRP_MSG){
    printf("Group: %d \n%s: %s\n",m->group_id,m->from,m->msg);
  }
}
