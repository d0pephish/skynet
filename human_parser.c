#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "macros.h"
/*
  definitions
*/
#define BUFFSIZE 4096

#define NONTERM	1
#define TERM	2
/*
  Structs:
    p_term_n  -> parser terminal node
    p_term    -> parser terminal
    p_nterm   -> parser non-terminal [is node]
    p_gram    -> parser grammar
    p_gram_w  -> parser grammar wrapper
*/

typedef struct
{
  char * value;
  uint32_t len;
} p_term;
struct p_term_n 
{
  p_term t;
  struct p_term_n * left;
  struct p_term_n * right;
  struct p_term_n * next;
};
typedef struct p_term_n p_term_n;

struct p_nterm 
{
  char * name;
  uint32_t n_len;
  char * value;
  uint32_t len;
  p_term_n * list;
  struct p_nterm * prev;
};
typedef struct p_nterm p_nterm;
typedef struct 
{
  char * name;
  uint32_t len;
  p_nterm * nterms;
} p_gram;

struct p_gram_w 
{
  p_gram * gram;
  char * value;
  uint32_t v_len;
  struct p_gram_w * next;
  struct p_gram_w * prev;
};
typedef struct p_gram_w p_gram_w;

/* prototypes */

p_gram * parse_grammar(char * g);

/* functions */

p_gram * create_grammar() 
{
  p_gram * g = ( p_gram * ) calloc( 1, sizeof(p_gram) );
  g->name = NULL;
  g->len = 0;
  g->nterms = NULL;
  return g;
}

char * clone_str( char * str, uint32_t len ) 
{
  if( len == 0 ) 
  {
    len = strlen( str );
  }
  char * ret = calloc( 1, len );
  memcpy( ret, str, len );
  return ret;
}

char * build_p_gram( char * sym, uint32_t len) 
{
  p_gram * new_p_gram = calloc( 1, sizeof(p_gram) );
  new_p_gram->name    = sym;
  new_p_gram->len     = len;
  new_p_gram->nterms  = NULL;
  return new_p_gram;
}



p_nterm * parse_grammer_line( char * line, uint32_t len, p_nterm * p_nterm_list ) 
{

  uint32_t i = 0;
  uint8_t type = TERM;

  p_nterm * cur = calloc( 1, sizeof( p_nterm ) );  

  char word[BUFFSIZE]; 


  char * line_value = calloc( 1, len );
  memcpy( line_value, line, len );
   
  cur->len = len;
  cur->value = line_value;
 
	 

  while ( (*line) != '\0' )
  {
    switch ( (*line) )
    {
      case '=':
	word[i] = '\0';

	cur->name = clone_str( &word , i);
	cur->len = i;

	i = 0;
	break;
      case ',':
	if( type == TERM ) 
	{

	} else if ( type == NONTERM )
	{

	}
        //should set type to TERM after 
        break;
      case '{':
        type = NONTERM;
        break;
        /// change to serach for } and finish. 
      case '}':
        break;
      default :
	buff[i] = (*line);
	i++;
        break;
    }
    line++;
  }
  return cur;
}

p_gram * parse_grammar( char * g_str ) 
{
  p_gram * g = create_grammar();
  p_gram * g_ptr = g;
  char *g_str_ptr = g_str;
 
  char buff[BUFFSIZE];
  uint32_t buff_i = 0;
  memset(&buff, 0, BUFFSIZE);
  
  while( (*g_str_ptr) != '\0' ) 
  {

    if ( (*g_str_ptr) == '\n' )
    {
      buff[buff_i] = '\0';
      // parse line
      printf( "%s\n", buff ); 
      memset( &buff, 0, BUFFSIZE );
      buff_i = 0;
      
    } else {
      if( buff_i >=BUFFSIZE )
      {
        debug("grammar too big. increase BUFFSIZE or shrink grammar.");
        break;
      }
      buff[buff_i] = (*g_str_ptr);
      buff_i++; 
    }
    g_str_ptr++;
  }
  printf( "%s", buff );
  return g;
}

int main() 
{
  char * grammar = "verb=are,is,were,was,will be\n" \
                   "noun={all}\n" \
                   "accept={noun}{verb}";

  parse_grammar( grammar );

}
