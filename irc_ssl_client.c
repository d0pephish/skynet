/*
 *  Based on the SSL client demonstration program
 *
 *  Copyright (C) 2006-2013, ARM Limited, All Rights Reserved
 *
 *  This file is part of mbed TLS (https://polarssl.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#define polarssl_printf     printf
#define polarssl_fprintf    fprintf
#endif

#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include "macros.h"
#include "parser.c"

#include "polarssl/net.h"
#include "polarssl/debug.h"
#include "polarssl/ssl.h"
#include "polarssl/entropy.h"
#include "polarssl/ctr_drbg.h"
#include "polarssl/error.h"
#include "polarssl/certs.h"

#if !defined(POLARSSL_BIGNUM_C) || !defined(POLARSSL_ENTROPY_C) ||  \
    !defined(POLARSSL_SSL_TLS_C) || !defined(POLARSSL_SSL_CLI_C) || \
    !defined(POLARSSL_NET_C) || !defined(POLARSSL_RSA_C) ||         \
    !defined(POLARSSL_CTR_DRBG_C) || !defined(POLARSSL_X509_CRT_PARSE_C)
int main( int argc, char *argv[] )
{
    ((void) argc);
    ((void) argv);

    polarssl_printf("POLARSSL_BIGNUM_C and/or POLARSSL_ENTROPY_C and/or "
           "POLARSSL_SSL_TLS_C and/or POLARSSL_SSL_CLI_C and/or "
           "POLARSSL_NET_C and/or POLARSSL_RSA_C and/or "
           "POLARSSL_CTR_DRBG_C and/or POLARSSL_X509_CRT_PARSE_C "
           "not defined.\n");
    return( 0 );
}
#else

#define SERVER_PORT 6697
#define SERVER_NAME "oxiii.net"
#define BOT_NAME "ircbot"
#define LOG_FILE "ircbot.logs"
#define DEBUG_LEVEL 1

//TO DO: wrap context in a struct so i only have to pass one object around rather than a ton or use globals.




int sender(ssl_context *, unsigned char *, int);



int alive = 1;
FILE * fp;


void cleanup_and_quit(int n) {
  alive = 0;
  fclose(fp);
}


static void my_debug( void *ctx, int level, const char *str )
{
    ((void) level);

    polarssl_fprintf( (FILE *) ctx, "%s", str );
    fflush(  (FILE *) ctx  );
}
void handle_INDV_PRIVMSG(char *msg, char *pos, ssl_context * ssl) {
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
  sender(ssl,(unsigned char *) reply,reply_len-1);
  free(received_word);
  free(reply);
}
void handle_PING(char * msg, char * pos, ssl_context * ssl) {
  debug("got a ping request");
  char response[4096] = "PONG";
  strncat((char *) &response,pos+4,sizeof(response) - 6);
  strncat((char *) &response,"\n",2);
  sender(ssl, (unsigned char *) &response, strlen(response));
}
void handle_CHAN_PRIVMSG(char * msg, char * pos, ssl_context * ssl) {
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
    sender(ssl,(unsigned char *) reply,reply_len-1);
    free(reply);
  } else {
    parse(pos);
    fwrite(pos,strlen(pos),sizeof(char), fp);
    fwrite("\n",1,1,fp);
  }
}

void parse_line(char * msg, ssl_context *ssl) {
  //debug( "got line: %s",msg);
  char * pos = NULL;
  if((pos= strstr(msg, "PING")) !=NULL) {
    handle_PING(msg,pos,ssl);
  } else if((pos=strstr(msg, "PRIVMSG #")) != NULL) {
    handle_CHAN_PRIVMSG( msg, pos+9, ssl);
  } else if((pos=strstr(msg, "PRIVMSG ")) != NULL) {
    handle_INDV_PRIVMSG( msg, pos+8, ssl);
  }
}

void handle_msg(char * msg, ssl_context * ssl) {
 char buf[4096];
 memset(&buf,0,4096);
 char * buf_ptr = (char *) &buf;
 char * msg_ptr = msg;
 int pos = 0;
 do
  {
  if(*msg_ptr == '\n') {
    *buf_ptr = 0x00;
    parse_line((char *) &buf, ssl);
    buf_ptr = (char *) &buf;
    msg_ptr++;
  } else {
    *buf_ptr = *msg_ptr;
    pos++;
    buf_ptr++;
    msg_ptr++;
  }  
  
  } while (*msg != 0x00 && *msg != EOF && pos<=sizeof(buf));
}
void *listener(void * ssl) {
      
      polarssl_printf( "  < Read from server:" );
      fflush( stdout );
      unsigned char buf[4096];
      int len, ret = -1;

      do
      {
          len = sizeof( buf ) - 1;
          memset( buf, 0, sizeof( buf ) );
          ret = ssl_read( ssl, buf, len );
          if( !alive ) break;

          if( ret == POLARSSL_ERR_NET_WANT_READ || ret == POLARSSL_ERR_NET_WANT_WRITE )
             continue;

          if( ret == POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY)
              break;

          if( ret < 0 )
          {
            polarssl_printf( "failed\n  ! ssl_read returned %d\n\n", ret );
            debug( "failed\n  ! ssl_read returned %d\n", ret );
            break;
          }

          if( ret == 0 )
          {
              polarssl_printf( "\n\nEOF\n\n" );
              break;
          }

          len = ret;
          polarssl_printf( " %d bytes read\n\n%s", len, (char *) buf );
          debug( " %d bytes read\n%s", len, (char *) buf );
          handle_msg( (char *) &buf, (ssl_context *) ssl);
      }
      while( alive );
      return NULL;
}

int sender(ssl_context * ssl, unsigned char * msg, int len) {
      debug("sender() is sending %d bytes of string |%s|",len,msg);
      int ret; 
      while( ( ret = ssl_write( ssl, msg, len ) ) <= 0 )
      {
          if( ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE )
          {
              polarssl_printf( " failed\n  ! ssl_write returned %d\n\n", ret );
              debug( " failed\n  ! ssl_write returned %d\n", ret );
              return -1;
          }
      }

      len = ret;
      polarssl_printf( " %d bytes written\n\n%s", len, msg );
      debug( " %d bytes written\n%s", len, msg );
      return len;
}
int main( int argc, char *argv[] )
{
    
    fp = fopen(LOG_FILE,"a+");
    parse_from_file(fp);
    int ret, len, server_fd = -1;
    unsigned char buf[4096];
    const char *pers = "irc_ssl_interactive_client";
    
    pthread_t lth;


    entropy_context entropy;
    ctr_drbg_context ctr_drbg;
    ssl_context ssl;
    x509_crt cacert;

    ((void) argc);
    ((void) argv);

    signal(SIGINT,cleanup_and_quit);
#if defined(POLARSSL_DEBUG_C)
    debug_set_threshold( DEBUG_LEVEL );
#endif

    /*
     * 0. Initialize the RNG and the session data
     */
    memset( &ssl, 0, sizeof( ssl_context ) );
    x509_crt_init( &cacert );

    polarssl_printf( "\n  . Seeding the random number generator..." );
    debug( "  . Seeding the random number generator..." );
    fflush( stdout );

    entropy_init( &entropy );
    if( ( ret = ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        polarssl_printf( " failed\n  ! ctr_drbg_init returned %d\n", ret );
        debug( " failed\n  ! ctr_drbg_init returned %d", ret );
        goto exit;
    }

    polarssl_printf( " ok\n" );

    /*
     * 0. Initialize certificates
     */
    polarssl_printf( "  . Loading the CA root certificate ..." );
    debug( "  . Loading the CA root certificate ..." );
    fflush( stdout );

#if defined(POLARSSL_CERTS_C)
    ret = x509_crt_parse( &cacert, (const unsigned char *) test_ca_list,
                          strlen( test_ca_list ) );
#else
    ret = 1;
    polarssl_printf("POLARSSL_CERTS_C not defined.");
    debug("POLARSSL_CERTS_C not defined.");
#endif

    if( ret < 0 )
    {
        polarssl_printf( " failed\n  !  x509_crt_parse returned -0x%x\n\n", -ret );
        debug( " failed\n  !  x509_crt_parse returned -0x%x\n", -ret );
        goto exit;
    }

    polarssl_printf( " ok (%d skipped)\n", ret );
    debug( " ok (%d skipped)", ret );

    /*
     * 1. Start the connection
     */
    polarssl_printf( "  . Connecting to tcp/%s/%4d...", SERVER_NAME, SERVER_PORT );
    debug( "  . Connecting to tcp/%s/%4d...", SERVER_NAME, SERVER_PORT );
    fflush( stdout );

    if( ( ret = net_connect( &server_fd, SERVER_NAME,
                                         SERVER_PORT ) ) != 0 )
    {
        polarssl_printf( " failed\n  ! net_connect returned %d\n\n", ret );
        debug( " failed\n  ! net_connect returned %d\n", ret );
        goto exit;
    }

    polarssl_printf( " ok\n" );
    debug( " ok\n" );

    /*
     * 2. Setup stuff
     */
    polarssl_printf( "  . Setting up the SSL/TLS structure..." );
    debug( "  . Setting up the SSL/TLS structure..." );
    fflush( stdout );

    if( ( ret = ssl_init( &ssl ) ) != 0 )
    {
        polarssl_printf( " failed\n  ! ssl_init returned %d\n\n", ret );
        debug( " failed\n  ! ssl_init returned %d\n", ret );
        goto exit;
    }

    polarssl_printf( " ok\n" );
    debug( " ok" );

    ssl_set_endpoint( &ssl, SSL_IS_CLIENT );
    /* OPTIONAL is not optimal for security,
     * but makes interop easier in this simplified example */
    ssl_set_authmode( &ssl, SSL_VERIFY_OPTIONAL );
    ssl_set_ca_chain( &ssl, &cacert, NULL, "PolarSSL Server 1" );

    /* SSLv3 is deprecated, set minimum to TLS 1.0 */
    ssl_set_min_version( &ssl, SSL_MAJOR_VERSION_3, SSL_MINOR_VERSION_1 );
    /* RC4 is deprecated, disable it */
    ssl_set_arc4_support( &ssl, SSL_ARC4_DISABLED );

    ssl_set_rng( &ssl, ctr_drbg_random, &ctr_drbg );
    ssl_set_dbg( &ssl, my_debug, stdout );
    ssl_set_bio( &ssl, net_recv, &server_fd,
                       net_send, &server_fd );

    /*
     * 4. Handshake
     */
    polarssl_printf( "  . Performing the SSL/TLS handshake..." );
    debug( "  . Performing the SSL/TLS handshake..." );
    fflush( stdout );

    while( ( ret = ssl_handshake( &ssl ) ) != 0 )
    {
        if( ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE )
        {
            polarssl_printf( " failed\n  ! ssl_handshake returned -0x%x\n\n", -ret );
            debug( " failed\n  ! ssl_handshake returned -0x%x\n", -ret );
            goto exit;
        }
    }

    polarssl_printf( " ok\n" );
    debug( " ok" );

    /*
     * 5. Verify the server certificate
     */
    polarssl_printf( "  . Verifying peer X.509 certificate..." );
    debug( "  . Verifying peer X.509 certificate..." );

    /* In real life, we may want to bail out when ret != 0 */
    if( ( ret = ssl_get_verify_result( &ssl ) ) != 0 )
    {
        polarssl_printf( " failed\n" );

        if( ( ret & BADCERT_EXPIRED ) != 0 )
            polarssl_printf( "  ! server certificate has expired\n" );
            debug( "  ! server certificate has expired" );

        if( ( ret & BADCERT_REVOKED ) != 0 )
            polarssl_printf( "  ! server certificate has been revoked\n" );
            debug( "  ! server certificate has been revoked" );

        if( ( ret & BADCERT_CN_MISMATCH ) != 0 )
            polarssl_printf( "  ! CN mismatch (expected CN=%s)\n", "PolarSSL Server 1" );
            debug( "  ! CN mismatch (expected CN=%s)", "PolarSSL Server 1" );

        if( ( ret & BADCERT_NOT_TRUSTED ) != 0 )
            polarssl_printf( "  ! self-signed or not signed by a trusted CA\n" );
            debug( "  ! self-signed or not signed by a trusted CA" );

        polarssl_printf( "\n" );
    }
    else
        polarssl_printf( " ok\n" );
        debug( " ok" );

    //begin client interactive prompt
    
    pthread_create(&lth,NULL, listener, (void *) &ssl);
    
    int again = 1;
    
    strncat((char *) &buf,"USER ",5);
    strncat((char*) &buf,BOT_NAME, min(strlen(BOT_NAME),sizeof(buf)-strlen((char *)&buf)-1));
    strncat((char *) &buf," * 0 :jianmin's irc bot\nNICK ",sizeof(buf)-strlen((char *) &buf)-1);
    strncat((char*) &buf,BOT_NAME, min(strlen(BOT_NAME),sizeof(buf)-strlen((char *)&buf)-1));
    strncat((char *) &buf, "\n",1);
    sender(&ssl, (unsigned char *) &buf, strlen((char *) &buf));

    while( again ) {
      polarssl_printf( "  Enter info to write to server:" );
      debug( "  Enter info to write to server:" );
      fflush( stdout );
      //len = sprintf( (char *) buf, GET_REQUEST );
      memset( (char *) &buf, 0, 4096);
      len = strlen(fgets((char *) &buf, 4095, stdin));

      if(strncmp((char *) &buf, "quit",4)==0) {
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
      
      if((ret = sender(&ssl, (unsigned char *) &buf, len))==-1) goto exit;
    }
    alive = 0;
    ssl_close_notify( &ssl );

exit:
    alive = 0;
#ifdef POLARSSL_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        polarssl_strerror( ret, error_buf, 100 );
        polarssl_printf("Last error was: %d - %s\n\n", ret, error_buf );
        debug("Last error was: %d - %s\n", ret, error_buf );
    }
#endif

    if( server_fd != -1 )
        net_close( server_fd );

    x509_crt_free( &cacert );
    ssl_free( &ssl );
    ctr_drbg_free( &ctr_drbg );
    entropy_free( &entropy );

    memset( &ssl, 0, sizeof( ssl ) );

#if defined(_WIN32)
    polarssl_printf( "  + Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif
    fclose(fp);
    return( ret );
}
#endif /* POLARSSL_BIGNUM_C && POLARSSL_ENTROPY_C && POLARSSL_SSL_TLS_C &&
          POLARSSL_SSL_CLI_C && POLARSSL_NET_C && POLARSSL_RSA_C &&
          POLARSSL_CTR_DRBG_C */
