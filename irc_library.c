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

#include "irc_structs.h"

#define SERVER_PORT 6697
#define SERVER_NAME "oxiii.net"
#define BOT_NAME "ircbot"
#define LOG_FILE "ircbot.logs"

//TO DO: wrap context in a struct so i only have to pass one object around rather than a ton or use globals.


/*


 This file provides support for parsing irc content


*/
/*
typedef struct {
  connection * c;
  char * server_name;
  char * bot_name;
  int server_port; 
} irc_session_node;
*/

extern unsigned long long int id_tracker;
extern struct word_node * word_list;


int alive = 1;
FILE * fp;



//PROTOTYPES
void parse_line(char *, irc_session_node *);
void handle_msg(char *, irc_session_node *, int, FILE *);
//void handle_INDV_PRIVMSG(char *, char *, connection *);
void handle_PING(char *, char *, connection *);
void handle_PRIVMSG(char *, char *, irc_session_node *);
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
/*
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
  char sent_seed[strlen(pos)+1];
  memset(&sent_seed,0,strlen(pos)+1);
  memcpy(&sent_seed,pos,strlen(pos));
  char * received_word = malloc(strlen(pos)+1);
  memset(received_word,0,strlen(pos)+1);
  memcpy(received_word,pos,strlen(pos)+1);
  char * sentence = build_sentence(received_word,word_list);
  int reply_len = strlen(sentence) + strlen(user)+12;
  char * reply = (char *) malloc(reply_len);
  memset(reply, 0, reply_len);
  snprintf(reply, reply_len, "PRIVMSG %s :%s\n",user, sentence);
  sender(c,(char *) reply,reply_len-1);
  free(received_word);
  free(reply);
}*/
void handle_PING(char * msg, char * pos, connection * c) {
  debug("got a ping request");
  char response[4096] = "PONG";
  strncat((char *) &response,pos+4,sizeof(response) - 6);
  strncat((char *) &response,"\n",2);
  sender(c, (char *) &response, strlen(response));
}
void handle_PRIVMSG(char * msg, char * pos, irc_session_node * sess) {
  char * user_pos;
  char * seed ;
  char * handle_pos;
  char * q_str = NULL;
  char * q_pos;
  int q_str_len;
  char * pre_reply = NULL;
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
//TODO: break this use case out into its own function. make a l33t parser. 
  if(pos[strlen(pos)-1] == '?') {
    pos[strlen(pos)-1] = '\0';
  } 
  if((handle_pos = strstr(pos,sess->bot_name)) != NULL) {
    debug(" found my name");
    if(handle_pos == pos || handle_pos == pos+1) {
      debug(" i'm being mentioned");
      if( (q_pos = strstr(handle_pos,"what is "))  != NULL) {
        debug("what is format");
        q_str_len = strlen(q_pos+8);
        q_str = calloc(1,q_str_len+1);
        memcpy(q_str,q_pos+8,q_str_len);
        pre_reply = calloc(1,q_str_len + 6 + strlen( (char *) &user));
        memcpy(pre_reply,&user,strlen(user));
        strncat(pre_reply,": ",2);
        strncat(pre_reply, q_str, q_str_len);
        strncat(pre_reply," is",3);
        seed = pre_reply;
      } else {
        seed = handle_pos + strlen(sess->bot_name)+1;
      }
      goto start_reply;
    }
    if(strstr(pos,">>")==pos) goto cmd_mode;
    else goto do_parse; 
  } else if(strstr(pos,">>")==pos || chan[0] != '#') {
    cmd_mode:
    if(chan[0] == '#') {
      seed = pos+2;
    } else {
      seed = pos;
    }
    start_reply:
    debug("User has submitted a message seed: %s",seed);
    char * received_word = calloc(1,strlen(seed)+1);
//    memset(received_word,0,strlen(seed)+1);
    memcpy(received_word,seed,strlen(seed));
    char * sentence = build_sentence(received_word,word_list);
//    char * sentence = build_sentence(gen_random_word_from_tree(),word_list);
    int reply_len = strlen(sentence) + strlen(chan)+12;
    char * reply = (char *) malloc(reply_len);
    memset(reply, 0, reply_len);
    if(chan[0]=='#') {
      snprintf(reply, reply_len, "PRIVMSG %s :%s\n", chan, sentence);
    } else {
      snprintf(reply, reply_len, "PRIVMSG %s :%s\n", user, sentence);
    }
    sender(sess->c,(char *) reply,reply_len-1);
    free(reply);
    free(sentence);
    free(received_word);
  } else {
    do_parse:
    parse(pos);
    fwrite(pos,strlen(pos),sizeof(char), fp);
    fwrite("\n",1,1,fp);
  }
  if(q_str!=NULL) free(q_str);
  if(pre_reply!=NULL) free(pre_reply);
}


void parse_line(char * msg, irc_session_node * sess) {
  //debug( "got line: %s",msg);
  char * pos = NULL;
  if((pos= strstr(msg, "PING")) !=NULL) {
    handle_PING(msg,pos,sess->c);
  } else if((pos=strstr(msg, "PRIVMSG ")) != NULL) {
    handle_PRIVMSG( msg, pos+9, sess);
  } /* else if((pos=strstr(msg, "PRIVMSG ")) != NULL) {
    handle_INDV_PRIVMSG( msg, pos+8, c);
  }*/
}

void handle_msg(char * msg, irc_session_node * sess, int len, FILE * extern_fp) {
 fp = extern_fp;
 char buf[4096];
 int buf_length = sizeof(buf);
 if(len<1) return;
 memset(&buf,0,4096);
 char * buf_ptr = (char *) &buf;
 char * msg_ptr = msg;
 int pos = 0;
  while (*msg_ptr != '\0' && *msg_ptr != EOF && pos<buf_length-1) { 
    if(*msg_ptr == '\n' || *msg_ptr == '\r') {
      *buf_ptr = '\0';
      parse_line((char *) &buf, sess);
      buf_ptr = (char *) &buf;
      msg_ptr++;
    } else {
      *buf_ptr = *msg_ptr;
      pos++;
      buf_ptr++;
      msg_ptr++;
    }  
  }
  buf[buf_length] = '\0';  
}
