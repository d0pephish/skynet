#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "macros.h"
/*
  definitions
*/
#define BUFFSIZE 4096

#define NONTERM 1
#define TERM	2
/*
  Structs:
    term_node  -> parser terminal node
    term    -> parser terminal
    non_term   -> parser non-terminal [is node]
    gram    -> parser grammar
    gram_w  -> parser grammar wrapper
*/

typedef struct
{
  char * value;
  uint32_t len;
} term;

struct term_node 
{
  term * t;
  struct term_node * left;
  struct term_node * right;
  struct term_node * next;
};
typedef struct term_node term_node;

struct non_term 
{
  char * name;
  uint32_t n_len;
  char * value;
  uint32_t len;
  term_node * list;
  struct non_term * prev;
};
typedef struct non_term non_term;
typedef struct 
{
  char * value;
  uint32_t value_len;
  non_term * nterms;
  gram_list * accept;
} gram;

struct gram_list 
{
  uint8_t type;
  struct gram_list * next;
  non_term * nt;
};
typedef struct gram_list gram_list;

struct gram_w 
{
  gram * gram;
  char * value;
  uint32_t v_len;
  struct gram_w * next;
  struct gram_w * prev;
};
typedef struct gram_w gram_w;
/* prototypes */

gram * parse_grammar(char * g);
non_term * parse_grammar_line( char * , uint32_t , non_term *  ); 


term_node * create_term_node( char * , uint32_t ); 
gram * build_gram( char * , uint32_t ); 
char * clone_str( char * , uint32_t ); 

void free_term(term * );

/* functions */

gram * create_grammar() 
{
  gram * g = ( gram * ) calloc( 1, sizeof(gram) );
  g->name = NULL;
  g->len = 0;
  g->nterms = NULL;
  return g;
}

term_node * create_term_node( char * val, uint32_t len) 
{
  term_node * new_term_node = (term_node *) calloc(1, sizeof(term_node) ); 
  new_term_node->t = (term *) calloc( 1, sizeof(term) );
  new_term_node->t->value = val;
  new_term_node->t->len = len;
  new_term_node->next = NULL;
  new_term_node->left = NULL;
  new_term_node->right = NULL;
  
  return new_term_node;
}

void free_term_node(term_node * old_term_node) 
{
  free( old_term_node->t->value );
  free( old_term_node->t );
  free( old_term_node );
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
non_term * build_non_term_single(char * name, uint32_t len)
{
  non_term * new_non_term = (non_term *) calloc( 1, sizeof(non_term) );
  new_non_term->name = name;
  new_non_term->n_len = len;
  new_non_term->value = NULL;
  new_non_term->len = 0;
  new_non_term->prev = NULL;
  new_non_term->list = create_term_node( name , len ); 
  return new_non_term;
}
gram * build_gram( char * sym, uint32_t len) 
{
  gram * new_gram = calloc( 1, sizeof(gram) );
  new_gram->name    = sym;
  new_gram->len     = len;
  term_node * create_term_node( char * , uint32_t ); 
  gram * build_gram( char * , uint32_t ); 
  new_gram->nterms  = NULL;
  return new_gram;
}
non_term * find_nterm_in_list(non_term * haystack, char * needle, uint32_t needle_len) {
  if (haystack == NULL)
    return NULL;
  uint32_t len = max(needle_len, haystack->len);
  int cmp_res = strncmp(needle, haystack->name, len);
  
  if(cmp_res == 0 ) 
  {
    return haystack;
  } else return find_nterm_in_list( haystack->prev, needle, needle_len );
}
void insert_non_term_into_list( non_term ** list, non_term * new ) 
{
  non_term * temp;
  if ( *list == NULL) 
    *list=new;
  else
  {
    temp = *list;
    *list = new;
    (*list)->prev = temp;
  }
}
void insert_term_node_into_list( term_node * list, term_node * term ) 
{
  if (list == NULL) 
    list = term;
  if ( list->t == NULL || term->t == NULL ) 
  {
    debug("empty t. this should not happen. check yourself.");
    return;  
  }
  if ( list->t->value == NULL || term->t->value == NULL) 
  {
    debug("empty string in t. this shouldn't happen.");
  }
  
  uint32_t max_len = max(term->t->len, list->t->len);
  int cmp_res = strncmp(term->t->value, term->t->value, max_len);
  if ( cmp_res == 0 ) 
  {
    debug("terminal is already in list. doing nothing.");
  } else if (cmp_res < 0) 
  {
    if ( list->left == NULL ) 
      list->left == term;
    else 
      insert_term_node_into_list( list->left, term );
  } else {
    if ( list->right == NULL )
      list->right == term;
    else 
      insert_term_node_into_list( list->right, term );
  }
}

non_term * parse_grammar_line( char * line, uint32_t len, non_term * non_term_list ) 
{
  uint32_t i = 0;
  uint8_t type = TERM;
  term_node * cur_term_node;
  non_term * cur_non_term;
  non_term * cur = calloc( 1, sizeof( non_term ) );  
  
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

	      cur->name = clone_str( (char *) word , i);
	      cur->len = i;

	      i = 0;
	      break;
      case ',':
	      word[i] = '\0';
	      if( type == TERM ) 
	      { // build terminal, create pointer to it. 
          cur_term_node = create_term_node( (char *) word, i);          
          insert_term_node_into_list( cur->list, cur_term_node );
	      } else if ( type == NONTERM )
	      { // find non-terminal in list, create pointer to it, or create new
          cur_non_term = find_nterm_in_list(non_term_list, word,i); 
          if( cur_non_term == NULL ) {
            cur_non_term = build_non_term_single( clone_str ( (char *) word, i), i ) ;
            // add non-terminal to list
            insert_non_term_into_list( &non_term_list, cur_non_term); 
          }           
          // insert into local list
          insert_term_node_into_list( cur->list, cur_term_node );
	      }
        
        i = 0; 
        //should set type to TERM after 
        break;
      case '{':
        type = NONTERM;
        break;
        /// change to serach for } and finish. 
      case '}':
        break;
      default :
        word[i] = (*line);
        i++;
        break;
    }
    line++;
  }
  return cur;
}

gram * parse_grammar( char * g_str ) 
{
  gram * g = create_grammar();
  gram * g_ptr = g;
  char *g_str_ptr = g_str;
  
  char buff[BUFFSIZE];
  uint32_t buff_i = 0;
  memset(&buff, 0, BUFFSIZE);
  
  while( (*g_str_ptr) != '\0' ) 
  {

    if ( (*g_str_ptr) == '\n' )
    {
      buff[buff_i] = '\0';
      parse_grammar_line( buff, buff_i, g_ptr->nterms ); 
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
