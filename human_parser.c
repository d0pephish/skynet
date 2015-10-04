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
    non_term   -> parser non-terminal
               -> non_terms of type TERM ahave term_nodes, of type NONTERM have offspring
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
  uint8_t type;
  uint32_t n_len;
  char * value;
  uint32_t len;
  struct non_term * offspring;
  term_node * list;
  struct non_term * next;
};
typedef struct non_term non_term;
typedef struct 
{
  char * value;
  char * name;
  uint32_t len;
  uint32_t value_len;
  non_term * nterms;
  struct gram_list * accept;
} gram;

struct gram_list 
{
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
char * parse_grammar_line( char * , uint32_t , non_term **  ); 


term_node * build_term_node( char * , uint32_t ); 
gram * build_gram( char * , uint32_t ); 
char * clone_str( char * , uint32_t ); 

void free_term(term * );

void debug_non_term(non_term *); 

/* functions */

gram * create_grammar() 
{
  debug("create_grammar()");
  gram * g = ( gram * ) calloc( 1, sizeof(gram) );
  g->name = NULL;
  g->len = 0;
  g->nterms = NULL;
  return g;
}

term_node * build_term_node( char * val, uint32_t len) 
{
  debug("build_term_node(%s,%d)",val,len);
  term_node * new_term_node = (term_node *) calloc(1, sizeof(term_node) ); 
  new_term_node->t = (term *) calloc( 1, sizeof(term) );
  new_term_node->t->value = clone_str(val,len);
  new_term_node->t->len = len;
  new_term_node->next = NULL;
  new_term_node->left = NULL;
  new_term_node->right = NULL;
  
  return new_term_node;
}

void free_term_node(term_node * old_term_node) 
{
  debug("free_term_node(%p)",old_term_node);
  free( old_term_node->t->value );
  free( old_term_node->t );
  free( old_term_node );
} 


char * clone_str( char * str, uint32_t len ) 
{
  debug("clone_str(%s,%d)",str,len);
  if( len == 0 ) 
  {
    len = strlen( str );
  }
  char * ret = calloc( 1, len+1 );
  memcpy( ret, str, len );
  return ret;
}
non_term * build_non_term_single(char * name, uint32_t len, uint8_t type)
{
  debug("build_non_term_single(%s,%d,%d)", name, len, type);
  non_term * new_non_term = (non_term *) calloc( 1, sizeof(non_term) );
  new_non_term->name = name;
  new_non_term->n_len = len;
  new_non_term->value = NULL;
  new_non_term->len = 0;
  new_non_term->next = NULL;
  new_non_term->offspring = NULL;
  new_non_term->type = type;
  new_non_term->list = build_term_node( name , len ); 
  return new_non_term;
}
gram * build_gram( char * sym, uint32_t len) 
{
  debug("build_gram(%s,%d)", sym, len);
  gram * new_gram     = calloc( 1, sizeof(gram) );
  new_gram->name      = sym;
  new_gram->value     = NULL;
  new_gram->value_len = 0; 
  new_gram->len       = len;
  new_gram->nterms    = NULL;
  new_gram->accept    = NULL;
  return new_gram;
}
non_term * find_non_term_in_list(non_term * haystack, char * needle, uint32_t needle_len) 
{
  debug("find_non_term_in_list(%p, %s, %d)", haystack, needle, needle_len);
  if (haystack == NULL)
    return NULL;
  uint32_t len = max(needle_len, haystack->len);
  int cmp_res = strncmp(needle, haystack->name, len);
  debug("find_non_term_in_list is comparing %s to %s", needle, haystack->name); 
  if(cmp_res == 0 ) 
  {
    return haystack;
  } else return find_non_term_in_list( haystack->next, needle, needle_len );
}
void insert_non_term_as_offspring( non_term * parent, non_term * child) {
  while(parent->offspring != NULL)
    parent = parent->offspring;
  parent->offspring = child;
}
void insert_non_term_into_list( non_term ** list, non_term * new ) 
{
  debug("insert_non_term_in_list(%p, %p)", list, new);
  non_term * temp;
  if ( *list == NULL) 
  {
    *list = new;
  } else
  {
    temp = *list;
    while ( temp->next != NULL ) 
    {
      temp = temp->next;
    }
    temp->next = new;
  }
/*
  if(*list->next == NULL)
    temp = *list;
    *list = new;
    (*list)->prev = temp;
  }*/
}
term_node * insert_term_node_into_list( term_node * list, term_node * term ) 
{
  debug("insert_term_node_into_list(%p,%p)",list,term);
  if (list == NULL) 
    return term;
  if ( list->t == NULL || term->t == NULL ) 
  {
    debug("empty t. this should not happen. check yourself.");
    return NULL;  
  }
  if ( list->t->value == NULL || term->t->value == NULL) 
  {
    debug("empty string in t. this shouldn't happen.");
    return NULL;
  }
  uint32_t max_len = max(term->t->len, list->t->len);
  int cmp_res = strncmp(term->t->value, list->t->value, max_len);
  if ( cmp_res == 0 ) 
  {
    debug("terminal is already in list. doing nothing.");
  } else if (cmp_res < 0) {
    debug("list->left: %p",list->left);
    if ( list->left == NULL ) 
      list->left = term;
    else 
      insert_term_node_into_list( list->left, term );
    debug("list->left: %p",list->left);
  } else {
    debug("list->right: %p",list->right);
    if ( list->right == NULL )
      list->right = term;
    else 
     insert_term_node_into_list( list->right, term );
    debug("list->right: %p",list->right);
  }
  return list;
}

char * parse_grammar_line( char * line, uint32_t len, non_term ** non_term_list ) 
{
  debug("parse_grammar_line(%s,%d,%p)",line,len,non_term_list);
  uint32_t i = 0;
  uint8_t type = TERM;
  term_node * cur_term_node;
  non_term * non_term_ptr;
  non_term * this_non_term = (non_term *) calloc( 1, sizeof( non_term ) );  
  
  char word[BUFFSIZE]; 

  
  char * line_value = calloc( 1, len );
  memcpy( line_value, line, len );
   
  this_non_term->len = len;
  this_non_term->value = line_value;
  this_non_term->next = NULL; 
	this_non_term->list = NULL;
  this_non_term->name = NULL;
  this_non_term->offspring = NULL;
  this_non_term->type = type;
  
   

  while ( (*line) != '\0' )
  {
    switch ( (*line) )
    {
      case '=':
	      word[i] = '\0';
	      this_non_term->name = clone_str( (char *) word , i);
	      this_non_term->n_len = i;
	      i = 0;
	      break;
      case '}':
        type = NONTERM;
      case ',':
	      word[i] = '\0';
      //TODO: fix the term vs. nonterm issue.
      // every line should be its own nonterm, and it should be made up of a list of nonterm elements that either point to other nonterms or point to list of terms.
	      if( type == TERM ) 
        {
          this_non_term->type = type;
          cur_term_node = build_term_node( word, i);
          this_non_term->list = insert_term_node_into_list( this_non_term->list, cur_term_node );
	      } else if ( type == NONTERM )
	      { // find non-terminal in list, create pointer to it, or create new
          non_term_ptr = find_non_term_in_list(*non_term_list, word, i);
          if(non_term_ptr == NULL) 
          {
            debug("non_term_ptr is NULL, creating a new non_terminal");
            non_term_ptr = build_non_term_single( clone_str ( (char *) word, i), i, TERM) ;
            // add non-terminal to list
            //insert_non_term_into_list( non_term_list, non_term_ptr); 
            //non_term_ptr = find_non_term_in_list(*non_term_list, word, i);
          }
            insert_non_term_as_offspring( this_non_term, non_term_ptr);
        }
        
        i = 0; 
        //should set type to TERM after 
        break;
      case '{':
        type = NONTERM;
        break;
      default :
        word[i] = (*line);
        i++;
        break;
    }
    line++;
  }
  debug("done building this_non_term, adding it ot list");  
  insert_non_term_into_list( non_term_list, this_non_term);
  return this_non_term->name;
}

gram * parse_grammar( char * g_str ) 
{
  debug("parse_grammar(%s)", g_str);
  gram * g = create_grammar();
  g->value_len = strlen(g_str);
  g->value = clone_str(g_str, g->value_len);
  char *g_str_ptr = g_str;
  
  char buff[BUFFSIZE];
  uint32_t buff_i = 0;
  memset(&buff, 0, BUFFSIZE);
  
  while( (*g_str_ptr) != '\0' ) 
  {

    if ( (*g_str_ptr) == '\n')
    {
      buff[buff_i] = '\0';
      g->name = parse_grammar_line( buff, buff_i, &( g->nterms) ); 
      // parse line
      //TODO: make this part of the code update the grammar list, don't do it in it's own function.
      //      much more efficient this way and then the offspring will be legitimate
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
  g->len = strlen(g->name);
  printf( "%s", buff );
  return g;
}

gram_list * build_gram_list(non_term * nt) 
{
  debug("build_gram_list(%p)", nt);
  gram_list * new_gram_list = calloc( 1, sizeof(gram_list) );
  new_gram_list->nt = nt;
  new_gram_list->next = NULL;
  return new_gram_list;
}
void debug_term_node(term_node * n) {
  if(n == NULL)
    return;
  debug("debug_term_node(%p)",n);
  debug("    n->t->value: %s",n->t->value);
  debug("    n->t->len: %d",n->t->len);
  debug("    n->left: %p",n->left);
  debug("    n->right: %p",n->right);
  debug("    n->next: %p",n->next);
  debug_term_node(n->left);
  debug_term_node(n->right);
  debug_term_node(n->next);
}
void debug_non_term_offspring(non_term * child) 
{
  if(child == NULL)
    return;
  debug("debug_non_term_offspring(%p)",child);
  debug("   child->name: %s", child->name);
  debug("   child->value: %s", child->value);
  debug("   child->list: %p", child->list);
  debug("   child->offspring: %p", child->offspring);
  term_node * list_ptr = child->list;
  while (list_ptr != NULL) {
    debug_term_node(list_ptr);
    list_ptr = list_ptr->next;
  }
  debug_non_term_offspring(child->offspring);
}
void debug_non_term(non_term * nt) 
{
  if(nt == NULL)
    return;

  debug("debug_non_term(%p)",nt);
  debug("  nt->name: %s",nt->name);
  debug("  nt->type: %d", nt->type);
  debug("  nt->n_len: %d", nt->n_len);
  debug("  nt->value: %s",nt->value);
  debug("  nt->len: %d", nt->len);
  debug("  nt->list: %p", nt->list);
  debug("  nt->next: %p", nt->next);
  debug("  nt->offspring: %p", nt->offspring);
  term_node * list_ptr = nt->list;
  while(list_ptr != NULL) 
  {
    debug_term_node(list_ptr);
    list_ptr = list_ptr->next; 
  }
  //debug_non_term(nt->next); 
  //debug_non_term_offspring(nt->offspring); 
}
void debug_gram_list(gram_list * l) 
{
  debug("debug_gram_list(%p)",l);
  if(l == NULL)
    return;
  
  debug(" l->next: %p",l->next);
  debug(" l->nt: %p", l->nt);
  debug(" l->nt->name: %s", l->nt->name);
  debug_non_term(l->nt);
  debug_gram_list(l->next);
}
void debug_grammar(gram * g) 
{
  debug("debug_grammar(%p)",g);
  if(g==NULL)
    return;
  debug("g->value: %s", g->value);
  debug("g->name: %s", g->name);
  debug("g->len: %d", g->len);
  debug("g->value_len: %d", g->value_len);
  debug("g->nterms: %p", g->nterms);
  debug("g->accept: %p", g->accept);
  debug_gram_list(g->accept);
}
void free_gram_list(gram_list * l) 
{
  debug("free_gram_list(%p)",l);
  free(l);
}
void populate_grammar_accept(gram * g) 
{
  debug("populate_grammar_accept(%p)",g);
  non_term * accept_non_term = find_non_term_in_list(g->nterms, g->name, g->len);
  non_term * accept_non_term_ptr = accept_non_term;

  if(accept_non_term == NULL || accept_non_term->offspring == NULL) 
  {
    debug("could not find accept line or accept line is empty.");
    return;
  }

  non_term * non_term_ptr = accept_non_term->offspring;
  g->accept = build_gram_list(non_term_ptr);
  
  gram_list * gram_list_ptr;
  gram_list_ptr = g->accept;

  non_term_ptr = non_term_ptr->offspring;
  while ( non_term_ptr != NULL ) {
    gram_list_ptr->next = build_gram_list(non_term_ptr);
    gram_list_ptr = gram_list_ptr->next;
    non_term_ptr = non_term_ptr->offspring;
  }
}

int main() 
{
  char * grammar = "verb=are,is,were,was,will be,\n" \
                   "noun=person,place,thing,\n" \
                   "accept={noun}{verb}\n";
  gram * g = parse_grammar( grammar );
  populate_grammar_accept(g);
  debug_grammar(g);
}
