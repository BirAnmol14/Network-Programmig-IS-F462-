#include "msgq.h"
int usr_cnt=0;
int grp_cnt=0;
User users[Max_USRS];
Group groups[Max_GRPS];
int membership[Max_GRPS][Max_USRS];
int serv;
char * myPath;
void addToPending(Message *m,User * u);
void sendPending(User * u);
void sendMessageToClient(User * u,Message * m);
int findClient(char * usr_name);
int getServer();
int getClient(char *);
void handleMsg(int );
void closeMyQueue(int);
void sendHistory(int id ,User * u,int cli);
void addToGrpHistory(Message *m,int id);
void handle(int signo){
  if(signo==SIGCHLD){
    wait(NULL);
  }
  if(signo == SIGINT){
    closeMyQueue(serv);
    exit(0);
  }
}

void die(char * s){
  perror(s);
  exit(1);
}

int main(){
  signal(SIGINT,handle);
 struct sigaction s_act;
 sigemptyset(&(s_act.sa_mask));
 s_act.sa_flags = SA_RESTART;
 s_act.sa_handler = handle;
 sigaction(SIGCHLD,&s_act,NULL);
 for(int i=0;i<Max_GRPS;i++){
   for(int j=0;j<Max_USRS;j++){
     membership[i][j]=0;
   }
 }
  serv = getServer();
  while(1){
    handleMsg(serv);
  }
  closeMyQueue(serv);
}

int getServer(){
  int serv = msgget(ftok(ServerQ,ProjID),IPC_EXCL|IPC_CREAT|S_IRUSR|S_IWUSR);
  if(serv<0){
    die("Unable to get server queue");
  }
  return serv;
}

int getClient(char * cli){
    if(cli == NULL){
      die("No client");
    }
    int c = msgget(ftok(cli,ProjID),IPC_CREAT|IPC_EXCL);
    if(errno==EEXIST){
      c= msgget(ftok(cli,ProjID),IPC_CREAT|S_IWUSR);
    }
    else if(c>0){
      puts("Client was not existent");
      msgctl(c,IPC_RMID,NULL);
      return -1;
    }
    return c;
}

void closeMyQueue(int id){
  int ret = msgctl(id,IPC_RMID,NULL);
  if(ret<0){
    die("Unable to remove Server queue");
  }
  return;
}

void addToPending(Message *m,User * u){
  if(u==NULL){
    return;
  }
  if(u->pendingHead == NULL){
    u->pendingHead = malloc(1*sizeof(History));
    u->pendingHead->message = malloc(1*sizeof(Message));
    u->pendingHead->message->mtype=m->mtype;
    u->pendingHead->message->type=m->type;
    strcpy( u->pendingHead->message->from,m->from);
    strcpy( u->pendingHead->message->to,m->to);
    u->pendingHead->message->group_id = m->group_id;
    strcpy(u->pendingHead->message->msg,m->msg);
    u->pendingHead->message->auto_delete=m->auto_delete;
    u->pendingHead->message->ts = m->ts;
    u->pendingHead->next=NULL;
    u->pendingTail=u->pendingHead;
    return;
  }
  u->pendingTail->next = malloc(1*sizeof(History));
  u->pendingTail = u->pendingTail->next;
  u->pendingTail->message = malloc(1*sizeof(Message));
  u->pendingTail->message->mtype=m->mtype;
  u->pendingTail->message->type=m->type;
  strcpy( u->pendingTail->message->from,m->from);
  strcpy( u->pendingTail->message->to,m->to);
  u->pendingTail->message->group_id = m->group_id;
  strcpy(u->pendingTail->message->msg,m->msg);
  u->pendingTail->message->auto_delete=m->auto_delete;
  u->pendingTail->message->ts = m->ts;
  u->pendingTail->next=NULL;
}

void sendPending(User * u){
  if(u->pendingHead==NULL){
    return;
  }
  History * t = u->pendingHead;
  History * prev;
  while(t!=NULL){
    sendMessageToClient(u,t->message);
    prev = t;
    t=t->next;
    free(prev);
  }
  u->pendingHead = NULL;
  u->pendingTail = NULL;
}

void sendMessageToClient(User * u,Message * m){
  int cli = getClient(u->user_name);
  if(m->auto_delete==TRUE){
    if(u->lastLogin - (m->ts).msg_time >= (m->ts).duration ){
      return;
    }

  }
    msgsnd(cli,m,sizeof(Message)-sizeof(long),0);
}
int findClient(char * usr_name){
  if(usr_name==NULL){
    return -1;
  }
  for(int i=0;i<usr_cnt;i++){
    if(strcmp(users[i].user_name,usr_name)==0){
      return i;
    }
  }
  return -1;
}


void handleMsg(int serv){
  Message m;
  memset(m.from,'\0',USR_NAME_MAX);
  memset(m.to,'\0',USR_NAME_MAX);
  memset(m.msg,'\0',MSG_MAX);
  Message m1;
  memset(m1.from,'\0',USR_NAME_MAX);
  memset(m1.to,'\0',USR_NAME_MAX);
  memset(m1.msg,'\0',MSG_MAX);
  int r =  msgrcv(serv,&m,sizeof(Message)-sizeof(long),0,0);
  if(r<0){
    return;
  }
  printf("%s to %s\n",m.from,m.to);
  int cli = getClient(m.from);
  if(m.type==LOGIN){
    int x = findClient(m.from);
    if(x<0){
        if(usr_cnt == Max_USRS){
          m1.mtype = 1;
          m1.type = RESPONSE;
          strcpy(m1.to,m.from);
          strcpy(m1.from,"system");
          strcpy(m1.msg,"Maximum User Limit Reached");
          msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
        }else{
          strcpy(users[usr_cnt].user_name,m.from);
          users[usr_cnt].isActive = TRUE;
          users[usr_cnt].lastLogin = time(NULL);
          users[usr_cnt].pendingHead = NULL;
          users[usr_cnt].pendingTail = NULL;

          m1.mtype = 1;
          m1.type = RESPONSE;
          strcpy(m1.to,m.from);
          strcpy(m1.from,"system");
          strcpy(m1.msg,"OK");
          msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
          sendPending(&users[usr_cnt]);
          usr_cnt++;
        }
    }
    else{
      User * u =&users[x];
      u->isActive = TRUE;
      u->lastLogin = time(NULL);
      m1.mtype = 1;
      m1.type = RESPONSE;
      strcpy(m1.to,m.from);
      strcpy(m1.from,"system");
      strcpy(m1.msg,"OK");
      msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
      sendPending(u);
    }
  }else if(m.type == LOGOUT){
    int x = findClient(m.from);
    if(x<0){
          m1.mtype = 1;
          m1.type = RESPONSE;
          strcpy(m1.to,m.from);
          strcpy(m1.from,"system");
          strcpy(m1.msg,"User was not present in system");
          msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
        }
    else{
      User * u =&users[x];
      u->isActive = FALSE;
      m1.mtype = 1;
      m1.type = RESPONSE;
      strcpy(m1.to,m.from);
      strcpy(m1.from,"system");
      strcpy(m1.msg,"OK");
      msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
    }
  }
  else if(m.type==SND_MSG){
    int sender = findClient(m.from);
    int receiver = findClient(m.to);
    if(receiver < 0){
      m1.mtype = 1;
      m1.type = RESPONSE;
      strcpy(m1.to,m.from);
      strcpy(m1.from,"system");
      strcpy(m1.msg,"No Such Receipient exists in system");
      msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
    }else{
      if(users[receiver].isActive==TRUE){
        printf("%s is Active\n",users[receiver].user_name);
        int cli1 = getClient(users[receiver].user_name);
        m.mtype = 2;
        msgsnd(cli1,&m,sizeof(Message)-sizeof(long),0);
      }else{
        printf("%s is not Active\n",users[receiver].user_name);
        addToPending(&m,&users[receiver]);
      }
      m1.mtype = 1;
      m1.type = RESPONSE;
      strcpy(m1.to,m.from);
      strcpy(m1.from,"system");
      strcpy(m1.msg,"OK");
      msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
    }
  }else if(m.type == LIST){
    m1.mtype = 1;
    m1.type = RESPONSE;
    strcpy(m1.to,m.from);
    strcpy(m1.from,"system");
    for(int i =0;i<usr_cnt;i++ ){
      sprintf(m1.msg,"User: %s (status: %d)",users[i].user_name,users[i].isActive==TRUE?1:0);
      msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
      memset(m1.msg,'\0',MSG_MAX);
    }
    for(int i=0;i<grp_cnt;i++){
      sprintf(m1.msg,"Group id: %d\nMembers:",groups[i].group_id);
      msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
      memset(m1.msg,'\0',MSG_MAX);
      for(int j=0;j<Max_USRS;j++){
        if(membership[groups[i].group_id][j] == 1){
          sprintf(m1.msg,"%s",users[j].user_name);
          msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
          memset(m1.msg,'\0',MSG_MAX);
        }
      }
    }
  }else if(m.type == CREATE_GRP){
    if(grp_cnt == Max_GRPS){
      m1.mtype = 1;
      m1.type = RESPONSE;
      strcpy(m1.to,m.from);
      strcpy(m1.from,"system");
      strcpy(m1.msg,"Maximum Group Limits Reached");
      msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
    }else{
      int x = findClient(m.from);
      if(x>=0){
        groups[grp_cnt].group_id = grp_cnt;
        groups[grp_cnt].historyHead = NULL;
        groups[grp_cnt].historyTail =NULL;
        membership[grp_cnt][x] = 1;
        m1.mtype = 1;
        m1.type = RESPONSE;
        strcpy(m1.to,m.from);
        strcpy(m1.from,"system");
        sprintf(m1.msg,"Group %d created, you were added to group",grp_cnt);
        msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
        grp_cnt++;
      }
    }
  }
  else if(m.type == JOIN_GRP){
      int id = m.group_id;
      if(id<0||id>=grp_cnt){
        m1.mtype = 1;
        m1.type = RESPONSE;
        strcpy(m1.to,m.from);
        strcpy(m1.from,"system");
        strcpy(m1.msg,"No Such Group Exists");
        msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
      }else{
        int x = findClient(m.from);
        if(x>=0){
          if(membership[id][x]==1){
            //already a member
            m1.mtype = 1;
            m1.type = RESPONSE;
            strcpy(m1.to,m.from);
            strcpy(m1.from,"system");
            strcpy(m1.msg,"You are already a member");
            msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
          }else{
            membership[id][x] = 1;
            m1.mtype = 1;
            m1.type = RESPONSE;
            strcpy(m1.to,m.from);
            strcpy(m1.from,"system");
            sprintf(m1.msg,"Added to Group %d successfully",id);
            msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
            printf("Sending History\n");
            sendHistory(id,&users[x],cli);
          }
        }
      }
  }else if(m.type == GRP_MSG){
      int gid = m.group_id;
      if(gid<0 || gid>=grp_cnt){
        m1.mtype = 1;
        m1.type = RESPONSE;
        strcpy(m1.to,m.from);
        strcpy(m1.from,"system");
        strcpy(m1.msg,"No Such Group Exists");
        msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
      }
      else{
          int x = findClient(m.from);
          if(membership[gid][x]==0){
            m1.mtype = 1;
            m1.type = RESPONSE;
            strcpy(m1.to,m.from);
            strcpy(m1.from,"system");
            strcpy(m1.msg,"You are not a member of the group");
            msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
          }else{
            addToGrpHistory(&m,gid);
            for(int i=0;i<Max_USRS;i++){
              if(membership[gid][i]==1 && strcmp(users[i].user_name,m.from)!=0){
                  //broadcast to all other members of group
                  if(users[i].isActive==TRUE){
                    printf("%s is Active\n",users[i].user_name);
                    int cli1 = getClient(users[i].user_name);
                    m.mtype = 2;
                    msgsnd(cli1,&m,sizeof(Message)-sizeof(long),0);
                  }else{
                    printf("%s is not Active\n",users[i].user_name);
                    addToPending(&m,&users[i]);
                  }
              }
            }
            m1.mtype = 1;
            m1.type = RESPONSE;
            strcpy(m1.to,m.from);
            strcpy(m1.from,"system");
            strcpy(m1.msg,"OK");
            msgsnd(cli,&m1,sizeof(Message)-sizeof(long),0);
          }
      }
  }
}

void addToGrpHistory(Message *m,int id){
  if((groups[id].historyHead) == NULL){
    (groups[id].historyHead) = malloc(1*sizeof(History));
    (groups[id].historyHead)->message = malloc(1*sizeof(Message));
    (groups[id].historyHead)->message->mtype=3;
    (groups[id].historyHead)->message->type=m->type;
    strcpy( (groups[id].historyHead)->message->from,m->from);
    strcpy( (groups[id].historyHead)->message->to,m->to);
    (groups[id].historyHead)->message->group_id = m->group_id;
    strcpy((groups[id].historyHead)->message->msg,m->msg);
    (groups[id].historyHead)->message->auto_delete=m->auto_delete;
    (groups[id].historyHead)->message->ts = m->ts;
    (groups[id].historyHead)->next=NULL;
    (groups[id].historyTail)=(groups[id].historyHead);
    printf("Added to history\n");
    return;
  }
  (groups[id].historyTail)->next = malloc(1*sizeof(History));
  (groups[id].historyTail) = (groups[id].historyTail)->next;
  (groups[id].historyTail)->message = malloc(1*sizeof(Message));
  (groups[id].historyTail)->message->mtype=3;
  (groups[id].historyTail)->message->type=m->type;
  strcpy( (groups[id].historyTail)->message->from,m->from);
  strcpy( (groups[id].historyTail)->message->to,m->to);
  (groups[id].historyTail)->message->group_id = m->group_id;
  strcpy((groups[id].historyTail)->message->msg,m->msg);
  (groups[id].historyTail)->message->auto_delete=m->auto_delete;
  (groups[id].historyTail)->message->ts = m->ts;
  (groups[id].historyTail)->next=NULL;
  printf("Added to history\n");
}

void sendHistory(int id,User * u,int cli){
  History * head = (groups[id].historyHead);
  if(head==NULL){
    printf("Head is null\n");
    return;
  }
  History * t = head;
  while(t!=NULL){
    if(t->message->auto_delete==TRUE){
      if(u->lastLogin - (t->message->ts).msg_time >= (t->message->ts).duration ){

      }else{
          msgsnd(cli,t->message,sizeof(Message)-sizeof(long),0);
      }
    }else{
        msgsnd(cli,t->message,sizeof(Message)-sizeof(long),0);
    }
    t=t->next;
  }
}
