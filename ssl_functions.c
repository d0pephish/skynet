#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include "macros.h"
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


//TO DO: wrap context in a struct so i only have to pass one object around rather than a ton or use globals.

typedef struct {
  int socket;
  SSL *ssl_handle;
  SSL_CTX *ssl_context;
} connection;

extern int alive;


//PROTOTYPES
int tcp_connect(char *, int);
connection * ssl_connect(char *, int);
void ssl_disconnect(connection *);
int ssl_write(connection *, char *, int);
int ssl_read(connection *, char **);
int sender(connection *, char *, int);

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


int sender(connection * c, char * msg, int len) {
  debug("sender() is sending %d bytes of string |%s|",len,msg);
      
  len = ssl_write(c, msg, len);
  if(len<=0)
  perror("error writing");
      
  printf(" %d bytes written:\n%s\n", len, msg );
  debug( " %d bytes written\n%s", len, msg );
  return len;
}

