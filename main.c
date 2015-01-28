#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// MACROS
#ifndef max
  #define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
  #define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif


//SETTINGS
int random_choice = 0;

//STRUCTS
struct branch {
  int score;
  struct word * item;
};
struct word {
  char * value;
  int len;
  struct branch ** next;
  int id;
  int option_count;
};
struct word_node {
  struct word * item;
  struct word_node * left;
  struct word_node * right;
};
//PROTOTYPES
struct word_node * build_node(struct word *);
struct word_node * insert_node(struct word *, struct word_node *);
void print_tree(struct word_node *);
struct word * build_word(char *, int);
//void prep_word_list(struct word_list *);
void parse(char *);
int valid_char(char);

//shared values

struct word_node * word_list = NULL;
//BEGIN
int main() {
parse("this is a test of the predictive text. If it works, this might tell you a pattern of what words to use. We'll havee to see if it works. I wonder if it will tell you what word comes next in the sentence or if it will be way off in its predictions. the more data there is, the better it will be. ");
}

void parse(char * input) {
  char * in_ptr = input;
  char buff[250];
  char * buff_ptr = (char *) &buff;
  
  int word_size = 0;

  memset(buff_ptr, 0, 250);
  while ((*in_ptr)!=0x00) {
    if (!valid_char((char) *in_ptr)) {
      printf("invalid char! skipping...\n");
    } else if(word_size >=249) {
      printf("word too big! stopping...\n");
      break;
    } else if (*in_ptr == ' ' || *in_ptr == '.') {
      if (word_size>0) {
        printf("%s\n",buff);
        word_list = insert_node(build_word(buff, word_size),word_list);
        //build_node(build_word(buff,word_size));
      }
      if (*in_ptr == '.') {
        printf(".\n"); 
      }
      buff_ptr= (char *) &buff;
      memset(buff_ptr, 0,250);
      word_size = 0;
    } else {
      *buff_ptr = tolower(*in_ptr);
      buff_ptr++;
      word_size++;
    }
    in_ptr++;
  }
  printf("done adding, now printing tree.\n");
  print_tree(word_list);
}

int valid_char(char c) {
  if(c<0x20) return 0;
  if(c>0x7e) return 0;
  return 1;
}

struct word * build_word(char * in, int len) {
  char * word_value = (char *) malloc(len+1);
  memcpy(word_value, in, len);
  *(word_value+len+1) = (char) 0x00;
  
  struct word * new_word = malloc(sizeof(struct word));
  memset(new_word,0,sizeof(struct word));
  new_word->value = word_value;

  new_word->len = len;
  
  return new_word;
}
/*
void prep_word_list(struct word_list * list) {
  if (list->max == 0) {

    struct word ** words = (struct word **) malloc(8);
    memset(words,0,8);

    list->max = 8;
    list->words = words;
  
  } else if(list->index==list->max) {

    struct word ** words = (struct word **) malloc(list->max * 2);
    memset(words, 0, list->max *2);
    memcpy(words, list->words,list->max*2);

    free(list->words);

    list->max = list->max * 2;

    list->words = words;
  }
}*/
struct word_node * build_node(struct word * item) {
  struct word_node * this_node = (struct word_node *) malloc(sizeof(struct word_node));
  memset(this_node, 0, sizeof(struct word_node));
  this_node->item = item;
  return this_node;
}

struct word_node * insert_node(struct word * new_word, struct word_node * tree_item) {
  if(tree_item == NULL) {
    tree_item = build_node(new_word);
  } else {
    int cmp_res = strncmp(tree_item->item->value,new_word->value,max(new_word->len,tree_item->item->len));
    if(cmp_res ==0) {
      // SAME WORD ALREADY HERE!
    } else if(cmp_res >0) {
      tree_item->left = insert_node(new_word, tree_item->left);
    } else {
      tree_item->right = insert_node(new_word, tree_item->right);
    }
    
  }
  return tree_item;
}

void print_tree(struct word_node * tree_node) {
  if(tree_node==NULL)
    return;
  if(tree_node->item == NULL) 
    return;
  print_tree(tree_node->left);
  printf("%s\n",tree_node->item->value);
  print_tree(tree_node->right);
}
