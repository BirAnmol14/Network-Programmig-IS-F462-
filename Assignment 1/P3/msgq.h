#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>

#define ServerQ "./msgq_server.c"
#define ProjID 'M'
#define Max_USRS 20
#define Max_GRPS 20
#define USR_NAME_MAX 128
#define MSG_MAX 256

typedef enum type{RESPONSE,SND_MSG, JOIN_GRP, LIST,CREATE_GRP,LOGIN,LOGOUT,GRP_MSG}Type;
typedef enum bool{TRUE,FALSE}Bool;
typedef struct time_stamp{
  time_t msg_time;
  int duration;
}timeStamp;
typedef struct message{
  long mtype;
  Type type;
  char from[USR_NAME_MAX];
  char to[USR_NAME_MAX];
  int group_id;
  char msg[MSG_MAX];
  Bool auto_delete;
  timeStamp ts;
}Message;

typedef struct history{
  Message * message;
  struct history * next;
}History;

typedef struct user{
  char user_name[USR_NAME_MAX];
  Bool isActive;
  time_t lastLogin;
  History * pendingHead;
  History * pendingTail;
}User;

typedef struct group{
  int group_id;
  History * historyHead;
  History * historyTail;
}Group;
