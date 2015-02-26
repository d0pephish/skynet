#include <string.h>
#include <stdio.h>
#include <signal.h>
#include "macros.h"
#include "parser_library.c"
#include "ssl_functions.c"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define SERVER_PORT 6697
#define SERVER_NAME "oxiii.net"
#define BOT_NAME "ircbot"
#define LOG_FILE "ircbot.logs"

//TO DO: wrap context in a struct so i only have to pass one object around rather than a ton or use globals.

typedef struct {
  connection * c;
  char * server_name;
  char * bot_name;
  int server_port; 
} irc_session_node;


extern unsigned long long int id_tracker;
extern struct word_node * word_list;


int alive = 1;
FILE * fp;



//PROTOTYPES
void parse_line(char *, connection *);
void handle_msg(char *, connection *, int, FILE *);
void handle_INDV_PRIVMSG(char *, char *, connection *);
void handle_PING(char *, char *, connection *);
void handle_CHAN_PRIVMSG(char *, char *, connection *);
void * extra_feature_handler(char *, int);
//FUNCTIONS
void * extra_feature_handler(char * input, int len) {
  int num;
  char * str;
  char * str2;
  if(strncmp(input,"parse file ", 11)==0) {
    str = input+11;
    num = strlen(str);
    if(num>0) {
      if(strstr(str,"\n")!=NULL) *(str+num-1) = '\0';
      FILE * new_fp;
      if((new_fp = fopen(str, "r+")) != NULL) {
        parse_from_file(new_fp); 
        fclose(new_fp);
        debug("%s parsed successfully", str);
        printf("%s parsed successfully\n", str);
      } else {
        debug("error opening file %s",str);
        printf("error opening file %s\nWill not load.",str);

      }
    }
  return (void *) 1;
  } else {
    return NULL;
  }
  
}

void handle_INDV_PRIVMSG(char *msg, char *pos, connection * c) {
  char * user_pos;
  if((user_pos = strstr(msg,"!"))==NULL) {
    debug("unexpected user string format");
    return;
  }
  char user[user_pos-msg]; // remember msg has : at beginning so there is a null byte accounted for
  memset(&user, 0, user_pos-msg);
  memcpy(&user,msg+1,user_pos-msg-1);

  msg = pos-1;
  if((pos = strstr(pos, ":"))==NULL) {
    debug("empty, malformed PRIVMSG received.");
    return;
  }
  char chan[pos-msg];
  memset(&chan, 0, pos-msg);
  memcpy(&chan, msg,pos-msg-1);
  pos++; 
  debug("%s to user: %s has msg: %s", (char *) &user, (char *) &chan, pos);
  fwrite(pos,strlen(pos),sizeof(char), fp);
  char sent_seed[strlen(pos)];
  memset(&sent_seed,0,strlen(pos));
  memcpy(&sent_seed,pos,strlen(pos)-1);
  char * received_word = malloc(strlen(pos));
  memset(received_word,0,strlen(pos));
  memcpy(received_word,pos,strlen(pos)-1);
  char * sentence = build_sentence(received_word,word_list);
  int reply_len = strlen(sentence) + strlen(user)+12;
  char * reply = (char *) malloc(reply_len);
  memset(reply, 0, reply_len);
  snprintf(reply, reply_len, "PRIVMSG %s :%s\n",user, sentence);
  sender(c,(char *) reply,reply_len-1);
  free(received_word);
  free(reply);
}
void handle_PING(char * msg, char * pos, connection * c) {
  debug("got a ping request");
  char response[4096] = "PONG";
  strncat((char *) &response,pos+4,sizeof(response) - 6);
  strncat((char *) &response,"\n",2);
  sender(c, (char *) &response, strlen(response));
}
void handle_CHAN_PRIVMSG(char * msg, char * pos, connection * c) {
  char * user_pos;
  if((user_pos = strstr(msg,"!"))==NULL) {
    debug("unexpected user string format");
    return;
  }
  char user[user_pos-msg]; // remember msg has : at beginning so there is a null byte accounted for
  memset(&user, 0, user_pos-msg);
  memcpy(&user,msg+1,user_pos-msg-1);

  msg = pos-1;
  if((pos = strstr(pos, ":"))==NULL) {
    debug("empty, malformed PRIVMSG received.");
    return;
  }
  char chan[pos-msg];
  memset(&chan, 0, pos-msg);
  memcpy(&chan, msg,pos-msg-1);
  pos++; 
  debug("%s in channel: %s has msg: %s", (char *) &user, (char *) &chan, pos);
  if(strstr(pos,">>")==pos) {
    debug("User has submitted a message seed: %s",pos+2);
    char * received_word = malloc(strlen(pos+2)+1);
    memset(received_word,0,strlen(pos+2)+1);
    memcpy(received_word,pos+2,strlen(pos+2));
    char * sentence = build_sentence(received_word,word_list);
//    char * sentence = build_sentence(gen_random_word_from_tree(),word_list);
    int reply_len = strlen(sentence) + strlen(chan)+12;
    char * reply = (char *) malloc(reply_len);
    memset(reply, 0, reply_len);
    snprintf(reply, reply_len, "PRIVMSG %s :%s\n", chan, sentence);
    sender(c,(char *) reply,reply_len-1);
    free(reply);
    free(sentence);
    free(received_word);
  } else {
    parse(pos);
    fwrite(pos,strlen(pos),sizeof(char), fp);
    fwrite("\n",1,1,fp);
  }
}


void parse_line(char * msg, connection * c) {
  //debug( "got line: %s",msg);
  char * pos = NULL;
  if((pos= strstr(msg, "PING")) !=NULL) {
    handle_PING(msg,pos,c);
  } else if((pos=strstr(msg, "PRIVMSG #")) != NULL) {
    handle_CHAN_PRIVMSG( msg, pos+9, c);
  } else if((pos=strstr(msg, "PRIVMSG ")) != NULL) {
    handle_INDV_PRIVMSG( msg, pos+8, c);
  }
}

void handle_msg(char * msg, connection * c, int len, FILE * extern_fp) {
 fp = extern_fp;
 char buf[4096];
 if(len<1) return;
 memset(&buf,0,4096);
 char * buf_ptr = (char *) &buf;
 char * msg_ptr = msg;
 int pos = 0;
  while (*msg_ptr != '\0' && *msg_ptr != EOF && pos<sizeof(buf)) { 
    if(*msg_ptr == '\n' || *msg_ptr == '\r') {
      *buf_ptr = '\0';
      parse_line((char *) &buf, c);
      buf_ptr = (char *) &buf;
      msg_ptr++;
    } else {
      *buf_ptr = *msg_ptr;
      pos++;
      buf_ptr++;
      msg_ptr++;
    }  
  }  
}
