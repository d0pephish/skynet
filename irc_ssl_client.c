#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include "macros.h"
#include "ssl_functions.c"
//#include "parser_library.c"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "irc_structs.h"

#define DYN_LIB "/ailib.so"
#define SERVER_PORT 6697
#define SERVER_NAME "oxiii.net"
#define BOT_NAME "skynet"
#define LOG_FILE "ircbot.logs"

/*

This is the IRC bubba that hangs out on IRC all of the time. It has some basic functionality and loads the actual bot modules into memory and executes them. allows for patching without join/quits in channel.

*/

//TO DO: wrap context in a struct so i only have to pass one object around rather than a ton or use globals.
/*
typedef struct {
  connection * c;
  char * server_name;
  char * bot_name;
  char * log_name;
  char * settings_filename;
  FILE * log_fp;
  pthread_t * lth;
  int server_port; 
} irc_session_node;
*/



int alive = 1;
void (*handle_msg)(char*, irc_session_node *, int, FILE *);
void *(*extra_feature_handler)(char *, int);
void (*parse_from_file)(FILE *);
FILE * fp;



//PROTOTYPES
void local_parse_line(char *, irc_session_node *);
void *listener(void *);
void local_handle_msg(char *, irc_session_node *, int);

//FUNCTIONS

void cleanup_and_quit(int n) {
  if(alive) {
    alive = 0;
    fclose(fp);
  }
}


void local_handle_PING(char * msg, char * pos, connection * c) {
  debug("got a ping request");
  char response[4096] = "PONG";
  strncat((char *) &response,pos+4,sizeof(response) - 6);
  strncat((char *) &response,"\n",2);
  sender(c, (char *) &response, strlen(response));
}

void local_parse_line(char * msg, irc_session_node * sess) {
  //debug( "got line: %s",msg);
  char * pos = NULL;
  if((pos= strstr(msg, "PING")) !=NULL) {
    local_handle_PING(msg,pos,sess->c);
  } 
}

void local_handle_msg(char * msg, irc_session_node * sess, int len) {
 char buf[4096];
 if(len<1) return;
 memset(&buf,0,4096);
 char * buf_ptr = (char *) &buf;
 char * msg_ptr = msg;
 int pos = 0;
  while (*msg_ptr != '\0' && *msg_ptr != EOF && pos<sizeof(buf)-1) { 
    if(*msg_ptr == '\n') {
      *buf_ptr = '\0';
      local_parse_line((char *) &buf, sess);
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
void *listener(void * sess) {
      
      char * data = NULL;
      int len = -1;

      do
      {
          if(!alive) break;
          len = ssl_read(((irc_session_node *) sess)->c, &data);
          
          if(len<1) {
            debug("socket likely closed. quitting");
            exit(0);
          }
          printf( " %d bytes read:\n%s\n", len, data );
          debug( " %d bytes read\n%s", len, data );
          if(handle_msg!=NULL) 
            handle_msg(data, (irc_session_node *) sess, len,fp); 
          else 
            local_handle_msg( data, (irc_session_node *) sess, len);
          
          free(data);
      }
      while( alive );
  return (NULL);
}

char * copy_str(char * input) {
  int len = strlen(input);
  char * out = malloc(len+2);
  memset(out,0,len+2);
  memcpy(out,input,len);
  return out;
}
irc_session_node * build_sess() {
  return NULL;
}
void free_sess(irc_session_node * sess) {
  if(sess->server_name) free(sess->server_name);
  if(sess->bot_name) free(sess->bot_name);
  if(sess->log_name) free(sess->log_name);
  if(sess->settings_filename) free(sess->settings_filename);
  free(sess);

  //TODO: fully impliment when you fully implement sessions
}

void * handle_settings_file(void * sess_in) {
  FILE * settings_file;
  irc_session_node * sess = (irc_session_node *) sess_in;
  sleep(1);
  if(sess->settings_filename) {
    if((settings_file = fopen(sess->settings_filename, "r"))!=NULL) {
      char settings_buf[1024];
      int read_len = 1;
      printf("sending this data:\n"); 
      while(read_len != 0) {
        memset((char *) &settings_buf, 0, 1024);
        read_len = fread((char *) &settings_buf, sizeof(char), 1023, settings_file);
        //printf("%s,%d",(char *) &settings_buf,read_len);
        sender(sess->c, (char *) &settings_buf, read_len);
        if(read_len<1023) break;
      }
      printf("--done sending settings file data.--\n");
    }      
  }
  return (NULL);
} 

int main( int argc, char *argv[] )
{

    extra_feature_handler = NULL;
    handle_msg = NULL; 
    parse_from_file = NULL;

    char lib_plus_cwd[1024];
    memset(&lib_plus_cwd,0,sizeof(lib_plus_cwd));
    if (getcwd(lib_plus_cwd, sizeof(lib_plus_cwd)) == NULL) {
      debug("could not get pwd.");
      memcpy(&lib_plus_cwd,DYN_LIB,strlen(DYN_LIB));
    } else {
      strncat(lib_plus_cwd,DYN_LIB,sizeof(lib_plus_cwd)-strlen(lib_plus_cwd)-1);
    }
    
    void *temp_lib_handle;
    void *lib_handle = dlopen (lib_plus_cwd, RTLD_NOW);
    if (!lib_handle) {
      fputs (dlerror(), stderr);
      printf("\nError: could not load external libraries. this is going to be pretty boring.\n");
    } else {
      printf("Loading library: %s\n", lib_plus_cwd);
      extra_feature_handler = dlsym(lib_handle, "extra_feature_handler");
      handle_msg = dlsym(lib_handle, "handle_msg");
      parse_from_file = dlsym(lib_handle, "parse_from_file");
    }

    
    irc_session_node * sess = (irc_session_node *) malloc(sizeof(irc_session_node));
    memset(sess,0,sizeof(irc_session_node));
    if(argc==6 || argc==2) {
      sess->settings_filename = copy_str(argv[argc-1]);
    } else {  
      sess->settings_filename = NULL;
    }
    if(argc>=5) {
      sess->server_name = copy_str(argv[1]);
      sess->server_port = atoi(argv[2]);
      sess->bot_name = copy_str(argv[3]);
      sess->log_name = copy_str(argv[4]);
    } else {
      printf(
      "Run without arguments, run again with:\n" \
      "  %s <servername> <portname> <botname> <logname>\n" \
      "to change defaults.\n\n", argv[0]);
      sess->server_port = SERVER_PORT;
      sess->server_name = (char *) copy_str((char *) &SERVER_NAME);
      sess->bot_name = (char *) copy_str((char *) &BOT_NAME);
      sess->log_name = (char *) copy_str((char *) &LOG_FILE);
    }
    printf("Running with:\nserver:\t\t\t%s\nport:\t\t\t%d\nlog file:\t\t%s\n",sess->server_name,sess->server_port,sess->log_name);
    if(sess->settings_filename)
      printf("settings:\t\t%s\n",sess->settings_filename);
    printf("\nLaunching in 2 seconds...\n");
    //getchar(); 
    sleep(2);
    fp = fopen(sess->log_name,"a+");
    if(parse_from_file != NULL) parse_from_file(fp);
    int ret, len = -1;
    char buf[4096] = "";
    
    pthread_t lth,settings_thread;

    connection * c = ssl_connect(sess->server_name,sess->server_port);

    sess->c = c;    

    signal(SIGINT,cleanup_and_quit);

    //begin client interactive prompt
     
    pthread_create(&lth,NULL, listener, (void *) sess);
    
    int again = 1;
    
    strncat((char *) &buf,"USER ",5);
    strncat((char*) &buf,sess->bot_name, min(strlen(sess->bot_name),sizeof(buf)-strlen((char *)&buf)-1));
    strncat((char *) &buf," * 0 :jianmin's irc bot\nNICK ",sizeof(buf)-strlen((char *) &buf)-1);
    strncat((char*) &buf,sess->bot_name, min(strlen(sess->bot_name),sizeof(buf)-strlen((char *)&buf)-1));
    strncat((char *) &buf, "\n",1);
    sender(c, (char *) &buf, strlen((char *) &buf));

    pthread_create(&settings_thread,NULL, handle_settings_file, (void *) sess);

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
      } else if(strncmp((char *) &buf, "fflush",6)==0) {
        debug("flushing...");
        fclose(fp);
        fp = fopen(sess->log_name,"a+");
        continue;
      } else if (strncmp((char *) &buf, "unload",6)==0) {
        debug("unloading. use reload to reload dynamic libraries");
        if(lib_handle)
          dlclose(lib_handle); 
        extra_feature_handler = NULL;
        handle_msg= NULL;
        parse_from_file = NULL;
        continue;
      } else if (strncmp((char *)&buf, "reload",6)==0) {
        debug("reloading...");
        extra_feature_handler = NULL;
        handle_msg = NULL; 
        parse_from_file = NULL;
        if(lib_handle)
          dlclose(lib_handle);
        lib_handle = dlopen (lib_plus_cwd, RTLD_NOW);
        if (!lib_handle) {
          fputs (dlerror(), stderr);
          printf("\nError: could not reload external libraries. Going into mute and deaf mode.\n");
        } else {
          printf("Loading library: %s\n", lib_plus_cwd);
          debug("Loading library: %s\n", lib_plus_cwd);
           
          extra_feature_handler = dlsym(lib_handle, "extra_feature_handler");
          handle_msg = dlsym(lib_handle, "handle_msg");
          parse_from_file = dlsym(lib_handle, "parse_from_file");
          
             
          if(parse_from_file != NULL) {
            rewind(fp);
            parse_from_file(fp);
          }
          //lib_handle = temp_lib_handle;
        }
        temp_lib_handle = NULL;
        continue;

      } else if(extra_feature_handler != NULL) {
        if( extra_feature_handler( (char *) &buf,len)!=NULL) {
          continue;
        }
      }
      
      if((ret = sender(c, (char *) &buf, len))<1) break;
    }
    cleanup_and_quit(0);
    pthread_join(lth,NULL);
    pthread_join(settings_thread,NULL);
    ssl_disconnect(c);
    if(sess)
      free_sess(sess);
    if(lib_handle)
      dlclose(lib_handle);
    return 0;
}
