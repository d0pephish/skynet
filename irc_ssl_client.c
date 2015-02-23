#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include "macros.h"
#include "parser.c"
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
  int socket;
  SSL *ssl_handle;
  SSL_CTX *ssl_context;
} connection;
typedef struct {
  connection * c;
  char * server_name;
  char * bot_name;
  int server_port; 
} irc_session_node;




int alive = 1;
FILE * fp;



//PROTOTYPES
int tcp_connect(char *, int);
connection * ssl_connect(char *, int);
void ssl_disconnect(connection *);
int ssl_write(connection *, char *, int);
int ssl_read(connection *, char **);
void parse_line(char *, connection *);
int sender(connection *, char *, int);
void *listener(void *);
void parse_line(char *, connection *);
void handle_msg(char *, connection *, int);
void handle_INDV_PRIVMSG(char *, char *, connection *);
void handle_PING(char *, char *, connection *);
void handle_CHAN_PRIVMSG(char *, char *, connection *);

//FUNCTIONS
int tcp_connect(char * serv, int port) {
  int error, handle;
  struct hostent *host;
  struct sockaddr_in server;
  
  host = gethostbyname(serv);
  handle = socket(AF_INET, SOCK_STREAM, 0);

  if (handle == -1) {
    perror("socket");
    handle = 0;
  } else {
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr =  *((struct in_addr *) host->h_addr);
    bzero(&(server.sin_zero),8);
    
    error = connect(handle, (struct sockaddr *) &server, sizeof(struct sockaddr));
  
    if (error == -1) {
      perror("connect");
      handle = 0;
    }

  }
  return handle;
}

connection *ssl_connect(char * server, int port) {
  connection *c;

  c = malloc(sizeof(connection));
  c->ssl_handle = NULL;
  c->ssl_context = NULL;

  c->socket = tcp_connect(server, port);

  if(c->socket) {
    SSL_load_error_strings();
    SSL_library_init();
    
    c->ssl_context = SSL_CTX_new(SSLv23_client_method());
    if (c->ssl_context == NULL) 
      ERR_print_errors_fp(stderr);
    
    c->ssl_handle = SSL_new (c->ssl_context);
    if(c->ssl_handle==NULL)
      ERR_print_errors_fp(stderr);
  
    if(!SSL_set_fd (c->ssl_handle, c->socket))
      ERR_print_errors_fp(stderr);

    if(SSL_connect(c->ssl_handle) != 1)
      ERR_print_errors_fp(stderr); 
  } else {
    perror ("Connect failed");
  }
  return c;
}

void ssl_disconnect(connection * c) {
  if (c->socket)
    close(c->socket);
  if(c->ssl_handle) {
    SSL_shutdown(c->ssl_handle);
    SSL_free(c->ssl_handle);
  }
  if(c->ssl_context)
    SSL_CTX_free(c->ssl_context);
  free(c);
}

int ssl_read(connection *c, char ** buff) {
  int read_size = 4096;
  char * rc = NULL; //malloc(read_size * sizeof(char) + 1);
  int len = 0;
  int received = 0;
  int count = 0;
  char buffer[read_size];
  memset((char *)&buffer, 0, read_size);
  
  if(c) {
    while(alive) {
      rc = (char *) realloc(rc, (count + 1) * read_size * sizeof(char) + 1);
      received = SSL_read(c->ssl_handle, buffer, read_size);
      buffer[received] = '\0';
      
      if(received > 0)
        memcpy(rc,&buffer,read_size);
        //rc = strncat (rc, (char *) &buffer,read_size);
      len += received;
      if(received < read_size)
        break;
      count++;
    }
  }
  rc[len+1] = '\0';
  *buff = rc;
  return len;
}

int ssl_write(connection * c, char * msg, int len) {
  if(c)
    return SSL_write(c->ssl_handle, msg, len);
  else
    return -1;
}






void cleanup_and_quit(int n) {
  if(alive) {
    alive = 0;
    fclose(fp);
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
    char * received_word = malloc(strlen(pos+2));
    memset(received_word,0,strlen(pos+2));
    memcpy(received_word,pos+2,strlen(pos+2)-1);
    char * sentence = build_sentence(received_word,word_list);
//    char * sentence = build_sentence(gen_random_word_from_tree(),word_list);
    int reply_len = strlen(sentence) + strlen(user)+12;
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

void handle_msg(char * msg, connection * c, int len) {
 char buf[4096];
 if(len<1) return;
 memset(&buf,0,4096);
 char * buf_ptr = (char *) &buf;
 char * msg_ptr = msg;
 int pos = 0;
  while (*msg_ptr != '\0' && *msg_ptr != EOF && pos<sizeof(buf)) { 
    if(*msg_ptr == '\n') {
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
void *listener(void * c) {
      
      char * data = NULL;
      int len = -1;

      do
      {
          if(!alive) break;
          len = ssl_read(c, &data);
          
          if(len<1) {
            debug("socket likely closed. quitting");
            exit(0);
          }
          printf( " %d bytes read:\n%s\n", len, data );
          debug( " %d bytes read\n%s", len, data );
          handle_msg( data, (connection *) c, len);
          free(data);
      }
      while( alive );
  return (NULL);
}

int sender(connection * c, char * msg, int len) {
  debug("sender() is sending %d bytes of string |%s|",len,msg);
      
  len = ssl_write(c, msg, len);
  if(len<=0)
  perror("error writing");
      
  printf(" %d bytes written:\n%s\n", len, msg );
  debug( " %d bytes written\n%s", len, msg );
  return len;
}
int main( int argc, char *argv[] )
{
    
    fp = fopen(LOG_FILE,"a+");
    parse_from_file(fp);
    int ret, len = -1;
    char buf[4096] = "";
    
    pthread_t lth;

    connection * c = ssl_connect(SERVER_NAME,SERVER_PORT);

    signal(SIGINT,cleanup_and_quit);

    //begin client interactive prompt
    
    pthread_create(&lth,NULL, listener, (void *) c);
    
    int again = 1;
    
    strncat((char *) &buf,"USER ",5);
    strncat((char*) &buf,BOT_NAME, min(strlen(BOT_NAME),sizeof(buf)-strlen((char *)&buf)-1));
    strncat((char *) &buf," * 0 :jianmin's irc bot\nNICK ",sizeof(buf)-strlen((char *) &buf)-1);
    strncat((char*) &buf,BOT_NAME, min(strlen(BOT_NAME),sizeof(buf)-strlen((char *)&buf)-1));
    strncat((char *) &buf, "\n",1);
    sender(c, (char *) &buf, strlen((char *) &buf));

    while( again ) {
      printf( "  Enter info to write to server:" );
      fflush( stdout );
      
      memset( (char *) &buf, 0, 4096);
      len = strlen(fgets((char *) &buf, 4095, stdin));

      if(strncmp((char *) &buf, "quit",4)==0) {
        sender(c, "QUIT\n",5);
        debug("quitting...");
        again = 0;
        continue;
      }
      if(strncmp((char *) &buf, "fflush",6)==0) {
        debug("flushing...");
        fclose(fp);
        fp = fopen(LOG_FILE,"a+");
        continue;
      }
      
      if((ret = sender(c, (char *) &buf, len))<1) break;
    }
    cleanup_and_quit(0);
    pthread_join(lth,NULL);
    ssl_disconnect(c);
    return 0;
}
