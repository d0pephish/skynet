#include "macros.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
/*

  This is version 0.1 of a predictive text tool. Every link between one word and the next is tracked, and multiple occurances of a link increases its score. 
  
  There are two phases to this tool: parsing and generation. 
    Parsing: every sentence run throgh parse() function is added to the database. You can call the function multiple times and it will affect the same tree.
    Generation: Give a one-word seed to build_sentence(). It will find the word in the list and walk its chain, producing a sentence. There is a function that will generate a random word for you, however it is really clunky and inefficient for now.
    CHANGELOG
    v0.1 supports parsing and tracking first order links between words. 
*/


//SETTINGS
int use_weights = 1;          // if 0, all words are treated as if they have a score of 0.
int starting_branch_size = 4; // starting memory allocated to array for branches. doubles every time it fills.
int minimum_score = 0;        // stop following branch if score gets below this value.
int no_repeats = 1;           // do not repeat a word
int buff_size = 250;
char empty_error[] = "word not trained\n";
//GLOBALS
extern unsigned long long id_tracker = 1;
extern struct word_node * word_list = NULL;
//STRUCTS
typedef struct phrase_id_node phrase_id_node;
typedef struct branch branch;
struct branch {
  int score;
  struct word * item;
  unsigned long long int id;
};
struct phrase_id_node {
  unsigned long long int id;
  int confidence;
  struct phrase_id_node * prev;
  struct phrase_id_node * next;
};
struct sentence_word {
  char * val;
  struct sentence_word * next;
  struct sentence_word * prev;
};
struct word {
  char * value;
  int len;
  int used;
  branch ** branches;
  branch ** prev;
  int max_branches;
  int branch_list_index;
  int max_prev;
  int prev_index;
};
struct word_node {
  struct word * item;
  struct word_node * left;
  struct word_node * right;
};
//PROTOTYPES
char * get_user_input(char *);
struct word * get_word_by_index(struct word_node *, int *);
int tree_size(struct word_node *);
char * gen_random_word_from_tree();
char * walk_link(struct word *,phrase_id_node *);
struct word * find_word(char *, struct word_node *);
char * build_sentence(char *, struct word_node *);
void print_branches(struct word *);
branch * build_branch(struct word *, unsigned long long);
void link_words(struct word *, struct word *);
void prep_branch_list(struct word *);
struct word_node * build_node(struct word *);
struct word_node * insert_str_as_node(struct word **, struct word_node *, char *, int);
void print_tree(struct word_node *);
struct word * build_word(char *, int);
//void prep_word_list(struct word_list *);
void parse(char *);
int valid_char(char);
struct word_node * clone_tree(struct word_node *);
void destroy_tree(struct word_node *);
struct word * clone_word(struct word *);
struct word_node * recursive_clone_tree(struct word_node *);
void fix_clone_links(struct word_node *, struct word_node *);
void parse_from_file(FILE *);
phrase_id_node * build_phrase_id_node(unsigned long long, int);
phrase_id_node * insert_phrase_id_node(phrase_id_node *, unsigned long long id, int);
void split_list_phrase_id_nodes(phrase_id_node *, phrase_id_node **, phrase_id_node **);
phrase_id_node * sorted_merge_phrase_id_nodes(phrase_id_node *, phrase_id_node *);
void merge_sort_phrase_id_nodes(phrase_id_node **);
struct sentence_word * build_sentence_word(char *, int);
struct sentence_word * process_sentence(char *);
void free_sentence_word(struct sentence_word *);
char * copy_sentence(char *);


struct word * clone_word(struct word * original) {
  struct word * new_word =  build_word(original->value,original->len);
  new_word->used              = original->used;
  new_word->max_branches      = original->max_branches;
  new_word->branch_list_index = original->branch_list_index;
  new_word->max_prev          = original->max_prev;
  new_word->prev_index        = original->prev_index;
  free(new_word->prev);
  free(new_word->branches);
  new_word->branches = (branch **) malloc(sizeof(branch *) * new_word->max_branches);
  new_word->prev = (branch **) malloc(sizeof(branch *) * new_word->max_prev);
  memcpy(new_word->branches,original->branches, sizeof(struct  branch *) * new_word->max_branches);
  memcpy(new_word->prev,original->prev, sizeof(struct  branch *) * new_word->max_prev);

  return new_word; 
}
void fix_clone_links(struct word_node * new, struct word_node * top) {
  int i=0;
  struct word * word_holder;
  branch * temp;
  if(new==NULL) return;
  while(i<new->item->branch_list_index) {
    temp = new->item->branches[i];
    word_holder = find_word(temp->item->value,top);
    if(word_holder==NULL) {
      debug("Cannot find word: temp->item->value:%s|", temp->item->value);
      exit(1);
    }
    new->item->branches[i] = build_branch(find_word(temp->item->value,top),temp->id);
    new->item->branches[i]->score = temp->score;
    i++;
  }
  i=0;
  while(i<new->item->prev_index) {
    temp = new->item->prev[i];
    new->item->prev[i] = build_branch(find_word(temp->item->value,top),temp->id);
    new->item->prev[i]->score = temp->score;
    i++;
  }
  fix_clone_links(new->left,top);
  fix_clone_links(new->right,top);
}
struct word_node * clone_tree(struct word_node * original) {
  struct word_node * new_tree = recursive_clone_tree(original);
  fix_clone_links(new_tree, new_tree);
  return new_tree;
}
struct word_node * recursive_clone_tree(struct word_node * original) {
  if (original == NULL) return NULL;
  struct word * new_word = clone_word(original->item);
  struct word_node * new_node = build_node(new_word);
  new_node->left = recursive_clone_tree(original->left);
  new_node->right = recursive_clone_tree(original->right);
  new_node->item = new_word;
  
  return new_node;
}

void destroy_tree(struct word_node * node) {
  if(node==NULL) return;
  int i = 0;
  destroy_tree(node->left);
  destroy_tree(node->right);
  while (i<node->item->branch_list_index) {
    free(node->item->branches[i]);
    i++;
  }
  i = 0;
  while(i<node->item->prev_index) {
    free(node->item->prev[i]);
    i++;
  }
  free(node->item->branches);
  free(node->item->prev);
  free(node->item->value);
  free(node->item);
  free(node);
  return;
}

char * get_user_input(char * msg) {
  printf("%s",msg);
  char * input = malloc(1024);
  memset(input, 0, 1024);
  char * input_ptr = input;
  int count = 0;
  char c;
  while((c = (char) getchar())!='\n' && count <1024) {
    *input_ptr = c;
    input_ptr++;
    count++;
  }
  if(count ==0) {
    free(input);
    return get_user_input("");
  }
  return input;
}
void parse_from_file(FILE * fp) {
    char buf[1024];
    char * buf_p = (char *) &buf;
    int count = 0;
    char c;
    memset(buf_p,0,1024);
     if(fp==NULL)
       exit(EXIT_FAILURE);
    while((c= fgetc(fp)) !=EOF) {
      if(count >=1023) {
        parse(buf);
        count = 0;
        buf_p = (char *) &buf;
        memset(buf_p, 0, 1024);
      }
      if(c!='\n' && c!= '\r') {
        *buf_p = c;
        buf_p++;
        count++;
      } else {
        debug("parsing: %s\n",buf);
        parse(buf);
        count = 0;
        buf_p = (char *) &buf;
        memset(buf_p, 0, 1024);
      }
    }
}
void parse(char * input) {
  char * in_ptr = input;
  char buff[buff_size];
  char * buff_ptr = (char *) &buff;
  struct word * prev_word = NULL;
  struct word * cur_word = NULL;
  struct word * cur_word_ptr[1]; 
  int word_size = 0;

  memset(buff_ptr, 0, 250);
  
  while ((*in_ptr)!=0x00) {
    
    if (!valid_char((char) *in_ptr)) {
      debug("invalid char! skipping.");    
    } else if(word_size >=(buff_size-1)) {
      debug("word too big! stopping.");
      break;
    
    } else if ((*in_ptr == ' ' || *in_ptr == '.' || *(in_ptr+1)==0x00) || *in_ptr == '!' || *in_ptr == '?') {
      if(*(in_ptr+1)==0x00 && (*in_ptr)!= ' ' && (*in_ptr)!= '.' && (*in_ptr)!='!' && *in_ptr != '?') {
        *buff_ptr = tolower(*in_ptr);
        word_size++;
      }
      if (*in_ptr == '.') {
        prev_word =NULL;
      }
      
      if (word_size>0) {
        word_list = insert_str_as_node((struct word **) &cur_word_ptr,word_list, buff, word_size);
        cur_word = cur_word_ptr[0];
        prep_branch_list(cur_word);
      
        if(prev_word != NULL && cur_word!=NULL) link_words(prev_word,cur_word);
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
    prev_word = cur_word;
  }
  
  id_tracker++;
  //print_tree(word_list);
}
phrase_id_node * build_phrase_id_node(unsigned long long id, int confidence) {
  phrase_id_node * node = (phrase_id_node *) malloc(sizeof(phrase_id_node));
  memset(node,0,sizeof(phrase_id_node));
  node->prev = NULL;
  node->next = NULL;
  node->id = id;
  node->confidence = confidence;
  return node;
}
phrase_id_node * insert_phrase_id_node(phrase_id_node * node, unsigned long long id, int confidence) {
  //phrase_id_node * front = node;
  if(node==NULL) return build_phrase_id_node(id,confidence);
  if(id==node->id) {
    node->confidence = node->confidence+ confidence;
  } else { 
    node->next = insert_phrase_id_node(node->next,id,confidence);
  }
  return node;
}
void split_list_phrase_id_nodes(phrase_id_node * src, phrase_id_node ** front_ref, phrase_id_node ** back_ref) {
  phrase_id_node * slow;
  phrase_id_node * fast;
  if(src==NULL || src->next==NULL) {
    *front_ref = src;
    *back_ref = NULL;
    return;
  }
  slow = src;
  fast = src->next;
  while(fast != NULL) {
    fast = fast->next;
    if(fast != NULL) {
      slow = slow->next;
      fast = fast->next;
    }
  }
  *front_ref = src;
  *back_ref = slow->next;
  slow->next = NULL;
}
phrase_id_node * sorted_merge_phrase_id_nodes(phrase_id_node * a, phrase_id_node * b) {
  phrase_id_node * result = NULL;
  
  if (a == NULL)
    return b;
  else if (b == NULL) 
    return a;

  if(a->id >= b->id) {
    result = a;
    result->next = sorted_merge_phrase_id_nodes(a->next,b);
  } else {
    result = b;
    result->next = sorted_merge_phrase_id_nodes(a, b->next);
  }
  return result;
}
void merge_sort_phrase_id_nodes(phrase_id_node ** head_ref) {
  phrase_id_node * head = *head_ref;
  phrase_id_node * a;
  phrase_id_node * b;
  if (head==NULL || head->next ==NULL) return;
  split_list_phrase_id_nodes(head, &a, &b);
  merge_sort_phrase_id_nodes(&a);
  merge_sort_phrase_id_nodes(&b);
  
  *head_ref = sorted_merge_phrase_id_nodes(a, b);
}
struct sentence_word * build_sentence_word(char * in, int len) {
  debug("build_sentence_word(): %s,%d",in,len);
  struct sentence_word * new_sent_word = (struct sentence_word *)  malloc(sizeof(struct sentence_word));
  memset(new_sent_word,0,sizeof(struct sentence_word));
  new_sent_word->val = malloc(len+1);
  memset(new_sent_word->val,0,len+1);
  memcpy(new_sent_word->val,in,len);
  new_sent_word->next = NULL;
  new_sent_word->prev = NULL;
  return new_sent_word;
}
struct sentence_word * process_sentence(char * input) {
  debug("process_sentence(): %s",input);
  int len = 0;
  struct sentence_word * cur = NULL;
  struct sentence_word * prev = NULL;
  char * input_ptr = input;
  char buff[buff_size];
  char * buff_ptr = (char *) &buff;
  memset(buff_ptr,0,buff_size);
  while( (*input_ptr)!= 0x00) {
    if(len>=buff_size) {
      debug("word too big for buffer");
    }
    if(( (*input_ptr) == ' ' || *(input_ptr+1)==0x00) && len>0) {
      if(*(input_ptr+1)==0x00) { 
        *buff_ptr = tolower((*input_ptr));
        len++;
      }
      
      cur = build_sentence_word((char *) &buff,len);
      buff_ptr = (char *) &buff;
      memset(buff_ptr,0,buff_size);  
      len = 0;
      if(prev!=NULL) prev->next = cur;
      cur->prev = prev;
      prev = cur; 
    } else {
      len++;
      *buff_ptr = tolower(*input_ptr);
      buff_ptr++;
    }
    input_ptr++;
  }
  return prev;
}
//frees from left to right
void free_sentence_word(struct sentence_word * in) {
  debug("free_sentence_word(): %p",in);
  if(in==NULL) return;
  free(in->val);
  free_sentence_word(in->next);
  free(in);
}
char * copy_sentence(char * input) {
  int len = strlen(input);
  char * out = malloc(len+1);
  memset(out,0,len+1);
  memcpy(out,input,len);
  return out;
}
char * build_sentence(char * input,struct word_node * orig_list) {
  debug("build_sentence(): %s,%p",input, orig_list);
  int len,i;
  int match = 0;
  //int confidence = 0;
  //unsigned long long best_id = 0;
  struct word_node * cur_list = clone_tree(orig_list);
  char * new_sentence;
  struct word * cur_word;
  struct word * temp;
  phrase_id_node * id_list = NULL;
  struct sentence_word * seed_sentence = process_sentence(input);
  if(seed_sentence == NULL) {
    debug("seed sentence is null");
    destroy_tree(cur_list);
    return copy_sentence( (char *) &empty_error);
  }
  debug("seed_sentence->val: %s",seed_sentence->val);
  struct word * starting_word = find_word(seed_sentence->val,cur_list); 
  debug("found word: %p",starting_word);
  cur_word = starting_word;
  if(starting_word == NULL) {
    debug("starting word is null");
    destroy_tree(cur_list);
    free_sentence_word(seed_sentence);
    return copy_sentence((char *) &empty_error);
  } else {
    debug("starting word is not null");
    while(seed_sentence->prev != NULL) {
    debug("in loop");
      seed_sentence = seed_sentence->prev;
      for(i=0;i<cur_word->prev_index;i++) {
        temp = cur_word->prev[i]->item;
        debug("comparing %s to %s",temp->value,seed_sentence->val);
        if(strncmp(temp->value,seed_sentence->val,max(temp->len,strlen(seed_sentence->val))) ==0) {
          debug("match!");
          match = 1;
          debug("adding id to list");
          id_list = insert_phrase_id_node(id_list,cur_word->prev[i]->id,1);
          debug("id list is: %p", id_list);
          cur_word = temp;
          break;
        }
      }
      if(!match) break;
      match = 0;
    }
    merge_sort_phrase_id_nodes(&id_list); 
    debug("id_list is %p",id_list);
    debug("walk_link using word %s",starting_word->value);
    char * pre_sentence = walk_link(starting_word,id_list);
    len = strlen(pre_sentence)+strlen(input)+1;
    new_sentence = malloc(len);
    memset(new_sentence,0,len);
    sprintf(new_sentence,"%s%s",input,pre_sentence);
    free(pre_sentence);
  }
  destroy_tree(cur_list);
  free_sentence_word(seed_sentence);
  return new_sentence;
}
char * walk_link(struct word * item, phrase_id_node * priorities) {
  debug("walk_link: %p,%p", item, priorities);
  
  phrase_id_node *priorities_ptr = priorities;
  branch * best_branch = NULL;
  branch * cur_branch = NULL;
  int best_score = INT_MIN;
  int i;
  char * deeper;
  char * cur_word;
  char * concat;
  int length;
  if(item==NULL) {
      debug("item is NULL");
      cur_word = malloc(2);
      memset(cur_word,0,1);
      return cur_word;
    }
  while(priorities_ptr!=NULL) {
    debug("inside priorities loop");
    for(i=0;i<item->branch_list_index;i++) {
      cur_branch = item->branches[i];
      //if(cur_branch==NULL) break; 
      if(cur_branch->id == priorities_ptr->id) {
        if(best_branch==NULL || rand()%4 == 1) {
          best_branch = cur_branch;
          best_score = cur_branch->score;
        }
      }
    }
    if(best_branch!=NULL) break;
    priorities_ptr= priorities_ptr->next;
  }
  if(best_branch==NULL) {
    debug("best branch is null");
    debug("on item: %s, with branch_list_index: %d",item->value,item->branch_list_index);
    for(i=0;i<item->branch_list_index;i++) {
      cur_branch = item->branches[i];
      if(cur_branch==NULL) break; 
      //debug("cur_branch->item: %p, cur_branch->id: %d, cur_branch->score: %d",cur_branch->item,cur_branch->id,cur_branch->score); 
      if((cur_branch->score > best_score || rand()%(1+best_score-cur_branch->score) == 0 ) && (cur_branch->item->used == 0 || no_repeats == 0)) {
        best_score = max(cur_branch->score,best_score);
        best_branch = cur_branch;
      } else if(cur_branch->score == best_score) { 
        if(rand()%2 == 1) {
          best_branch = cur_branch;
        }
      }
    }
  }
  if(best_score < minimum_score) {
    cur_word = malloc(3);
    memset(cur_word, 0, 3);
    memcpy(cur_word, ".\n",2);
    return cur_word;
  } else {
    cur_word = best_branch->item->value;
    best_branch->score--;
    best_branch->item->used = 1;
  }
  if(best_branch!=NULL) {
    deeper = walk_link(best_branch->item,priorities);
  } else deeper = walk_link(NULL,priorities);
  length = strlen(deeper)+strlen(cur_word)+5;
  concat = malloc(length);
  memset(concat, 0, length);
  sprintf(concat," %s%s", cur_word, deeper);
  if(deeper != NULL) free(deeper);
  return concat;
}
struct word * find_word(char * input, struct word_node * item) {
//  debug("find_word %s, %p", input, item);
  if(item==NULL) return NULL;
  int cmp = strncmp(item->item->value,input,max(item->item->len,strlen(input)));
  if(cmp==0) {
    return item->item;
  } else if(cmp>0) {
    return find_word(input, item->left);
  } else {
    return find_word(input, item->right);
  }
}
int valid_char(char c) {
  if(c<0x20) return 0;
  if(c>0x7e) return 0;
  return 1;
}

branch * build_branch(struct word * item, unsigned long long id) {
  
  branch * this_branch = (branch *) malloc(sizeof(branch));
  memset(this_branch, 0, sizeof(branch));

  this_branch->item = item;
  this_branch->score = 1;
  this_branch->id = id;

  return this_branch;
}
void link_words(struct word * prev, struct word * cur) {
  debug("link_words: %p, %p", prev, cur);
  int i,cmp;
  int dont_link = 0;
  struct word * branch_list_word;
  if(prev==NULL) return;
  
  prep_branch_list(prev);
  
  for(i = 0; i<prev->branch_list_index; i++) {
    branch_list_word = prev->branches[i]->item;
    cmp = strncmp(branch_list_word->value, cur->value, max(branch_list_word->len, cur->len));
    if(cmp==0) {
      if(use_weights==1) prev->branches[i]->score = prev->branches[i]->score + 1;
      dont_link = 1;
    }
  }
  if(!dont_link) {
    prev->branches[prev->branch_list_index] = build_branch(cur, id_tracker);
    prev->branch_list_index++;
  }
  for(i = 0; i<cur->prev_index; i++) {
    branch_list_word = cur->prev[i]->item;
    cmp = strncmp(branch_list_word->value, prev->value, max(branch_list_word->len, prev->len));
    if(cmp==0) {
      if(use_weights==1) cur->prev[i]->score = cur->prev[i]->score + 1;
      return;
    }
  }
  cur->prev[cur->prev_index] = build_branch(prev, id_tracker);
  cur->prev_index++;
}
struct word * build_word(char * in, int len) {
  //debug("build_Word %s, %d", in, len);
  if(len==0 || strlen(in)==0) {
    debug("\n\n\n\n\n\n~~~~~~~!!!!!!!!!!!~~~~~~~~~\n\n\n\n\n\n");
  }
  char * word_value = (char *) malloc(len+1);
  memset(word_value, 0, len+1);
  memcpy(word_value, in, len);
  
  struct word * new_word = (struct word *) malloc(sizeof(struct word));
  memset(new_word,0,sizeof(struct word));
  new_word->value = (char *) word_value;
  new_word->used = 0;
  new_word->len = len;
  new_word->max_branches = starting_branch_size;
  new_word->max_prev = starting_branch_size;
  new_word->prev_index = 0;
  new_word->branch_list_index = 0;
  new_word->branches = (branch **) malloc(starting_branch_size*sizeof(branch *));
  memset(new_word->branches, 0, starting_branch_size*sizeof(branch *));
  new_word->prev = (branch **) malloc(starting_branch_size*sizeof(branch *));
  memset(new_word->prev, 0, starting_branch_size*sizeof(branch *));

  return new_word;
}

void prep_branch_list(struct word * cur_word) {
//  debug("prep_branch_list %p",cur_word);
  if(cur_word->branch_list_index>=cur_word->max_branches) {
    cur_word->max_branches = cur_word->max_branches * 2;
    cur_word->branches = (branch **) realloc(cur_word->branches, cur_word->max_branches*sizeof(branch *));
  }
  if(cur_word->prev_index>=cur_word->max_prev) {
    cur_word->max_prev = cur_word->max_prev * 2;
    cur_word->prev = (branch **) realloc(cur_word->prev, cur_word->max_prev*sizeof(branch *));
  }
}
struct word_node * build_node(struct word * item) {
//  debug("build_node: %p",item);
  struct word_node * this_node = (struct word_node *) malloc(sizeof(struct word_node));
  memset(this_node, 0, sizeof(struct word_node));
  this_node->left = NULL;
  this_node->right = NULL;
  this_node->item = item;
  return this_node;
}

struct word_node * insert_str_as_node(struct word ** new_word_ptr, struct word_node * tree_item, char * buff, int buff_len) {
//  debug("insert_str_as_node: %p,%p,%s,%d",new_word_ptr, tree_item,buff,buff_len);
  if(tree_item == NULL) {
    new_word_ptr[0] = build_word(buff,buff_len);
    tree_item = build_node(new_word_ptr[0]);
  } else {
    int cmp_res = strncmp(tree_item->item->value,buff,max(buff_len,tree_item->item->len));
    if(cmp_res ==0) {
      new_word_ptr[0] = tree_item->item;
    } else if(cmp_res >0) {
      tree_item->left = insert_str_as_node(new_word_ptr, tree_item->left, buff, buff_len);
    } else {
      tree_item->right = insert_str_as_node(new_word_ptr, tree_item->right, buff, buff_len);
    }
    
  }
  return tree_item;
}
char * gen_random_word_from_tree() {
  debug("gen_random_word_from_tree()");
  int max_depth = tree_size(word_list);
  int ran = rand() % max_depth;
  struct word * ran_word = NULL;
  while(ran_word == NULL) {
    ran_word = get_word_by_index(word_list, (int *) &ran);
    ran = rand() % max_depth;
  }
  return ran_word->value;
}
struct word * get_word_by_index(struct word_node * tree_node, int * i) {
  debug("get_word_by_index, %p,%p(%i)",tree_node, i, *i);
  if (tree_node== NULL) return NULL;
  if (*i<=1) return tree_node->item;
  (*i)--;
  struct word * result = get_word_by_index(tree_node->left, i);
  if(result != NULL) return result;
  result = get_word_by_index(tree_node->right, i);
  if(result != NULL) return result;
  return tree_node->item;
  
}
int tree_size(struct word_node * tree_node) {
  if (tree_node==NULL) return 0;
  return 1 + max(tree_size(tree_node->left), tree_size(tree_node->right));
}
void print_tree(struct word_node * tree_node) {
  if(tree_node==NULL)
    return;
  if(tree_node->item == NULL) 
    return;
  print_tree(tree_node->left);
  debug("%s (%d):",tree_node->item->value,tree_node->item->branch_list_index);
  print_branches(tree_node->item);
  debug("\n");
  print_tree(tree_node->right);
}
void print_branches(struct word * item) {
  int i;
  for(i=0; i<item->branch_list_index; i++) {
    debug("->%s,%d;",item->branches[i]->item->value, item->branches[i]->score);
  }
}
