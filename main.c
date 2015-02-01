#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
/*

  This is version 0.1 of a predictive text tool. Every link between one word and the next is tracked, and multiple occurances of a link increases its score. 
  
  There are two phases to this tool: parsing and generation. 
    Parsing: every sentence run throgh parse() function is added to the database. You can call the function multiple times and it will effect the same tree.
    Generation: Give a one-word seed to build_sentence(). It will find the word in the list and walk its chain, producing a sentence. There is a function that will generate a random word for you, however it is really clunky and inefficient for now.
    CHANGELOG
    v0.1 supports parsing and tracking first order links between words. 




*/
// MACROS
#ifndef max
  #define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
  #define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

#ifndef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

//SETTINGS
int use_weights = 1;
int starting_branch_size = 2;
int minimum_score = 5;
//STRUCTS
struct branch {
  int score;
  struct word * item;
};//TO DO: add branch functionality
struct word {
  char * value;
  int len;
  struct branch ** branches;
  int max_branches;
  int branch_list_index;
};
struct word_node {
  struct word * item;
  struct word_node * left;
  struct word_node * right;
};
//PROTOTYPES
struct word * get_word_by_index(struct word_node *, int *);
int tree_size(struct word_node *);
char * gen_random_word_from_tree();
void walk_link(struct word *);
struct word * find_word(char *, struct word_node *);
struct branch * build_branch(struct word *);
void build_sentence(char *);
void print_branches(struct word *);
struct branch * build_branch(struct word *);
void link_words(struct word *, struct word *);
void prep_branch_list(struct word *);
struct word_node * build_node(struct word *);
struct word_node * insert_str_as_node(struct word **, struct word_node *, char *, int);
void print_tree(struct word_node *);
struct word * build_word(char *, int);
//void prep_word_list(struct word_list *);
void parse(char *);
int valid_char(char);

//shared values

struct word_node * word_list = NULL;
//BEGIN
int main(int argc, char * argv[]) {
  FILE *fp;
  if(argc<=1) {
  printf("must give file to read from: %s <filename>. Running on sample data.\n",argv[0]);
  parse("A comparative statement has the format if then. this is a test of the predictive text. If it works, this might tell you a pattern of what words to use. We'll havee to see if it works. I wonder if it will tell you what word comes next in the sentence or if it will be way off in its predictions. the more data there is, the better it will be. ");
  } else {
    fp = fopen(argv[1], "r");
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
  srand(time(NULL));
  build_sentence("she");
  build_sentence(gen_random_word_from_tree());
}

void parse(char * input) {
  char * in_ptr = input;
  char buff[250];
  char * buff_ptr = (char *) &buff;
  struct word * prev_word = NULL;
  struct word * cur_word = NULL;
  struct word * cur_word_ptr[1]; 
  int word_size = 0;

  memset(buff_ptr, 0, 250);
  while ((*in_ptr)!=0x00) {

    if (!valid_char((char) *in_ptr)) {
  
    //  printf("invalid char! skipping...\n");
  
    } else if(word_size >=249) {
      
      debug("word too big! stopping...\n");
      break;
    
    } else if (*in_ptr == ' ' || *in_ptr == '.') {
      
      if (*in_ptr == '.') {
        prev_word =NULL;
   //     printf(".\n"); 
      
      }
      
      if (word_size>0) {
        
  //      printf("%s\n",buff);
        //cur_word = build_word(buff, word_size);
        //printf("cur word before:\t%p\n",cur_word);
        word_list = insert_str_as_node((struct word **) &cur_word_ptr,word_list, buff, word_size);
        cur_word = cur_word_ptr[0];
        //printf("cur_word after:\t%p\n",cur_word);
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
  //printf("done adding, now printing tree.\n");
  //print_tree(word_list);
}
void build_sentence(char * input) {
  printf("build_sentence: %s",input);
  struct word * starting_word = find_word(input,word_list); 
  if(starting_word == NULL) {
    printf("\nno links to follow with this input. quitting\n");
    return;
  }
  //print_branches(starting_word);
  walk_link(starting_word);
}
void walk_link(struct word * item) {
  debug("walk_link");
  //print_branches(item);
  if(item==NULL) return; 
  debug("walk_link item value %s", item->value);
  struct branch * best_branch;
  struct branch * cur_branch;
  int best_score = INT_MIN;
  int i;
  for(i=0;i<item->branch_list_index;i++) {
    cur_branch = item->branches[i];
    //debug("walk_link loop on %s",item->branches[i]->item->value);
    if(cur_branch==NULL) debug("walk_link found an item to be null unexpectedly");
    if(cur_branch==NULL) return;
    if(cur_branch->item==NULL) debug("SOMETHING WICKED HAPPENED HERE ! \n");
    if(cur_branch->score > best_score) {
      best_score = cur_branch->score;
      best_branch = cur_branch;
    } else if(cur_branch->score == best_score) { 
      if(rand()%2 == 1) {
        best_branch = cur_branch;
      }
    }
  }
  if(best_score < minimum_score) {
    printf(".\n");
    return;
  }
  printf(" %s",best_branch->item->value);
  best_branch->score--;
  walk_link(best_branch->item);
}
struct word * find_word(char * input, struct word_node * item) {
  debug("find_word %s, %p", input, item);
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

struct branch * build_branch(struct word * item) {
  
  struct branch * this_branch = (struct branch *) malloc(sizeof(struct branch));
  memset(this_branch, 0, sizeof(struct branch));

  this_branch->item = item;
  this_branch->score = 1;

  return this_branch;
}
void link_words(struct word * prev, struct word * cur) {
  debug("link_words: %p, %p", prev, cur);
  int i,cmp;
  struct word * branch_list_word;
  if(prev==NULL) return;
  
  debug("link_words: before: %p index: %d; max: %d",prev->branches, prev->branch_list_index,prev->max_branches);
  print_branches(prev);
  prep_branch_list(prev);
  print_branches(prev);
  
  debug("link_words: after: %p index: %d; max: %d",prev->branches, prev->branch_list_index,prev->max_branches);
  //check if word is already in a branch, if not, add it. 
  for(i = 0; i<prev->branch_list_index; i++) {
    branch_list_word = prev->branches[i]->item;
    //prep_branch_list(branch_list_word);
    //branch_list_word = (*(prev->branches+i))->item;
    cmp = strncmp(branch_list_word->value, cur->value, max(branch_list_word->len, cur->len));
    if(cmp==0) {
      if(use_weights==1) prev->branches[i]->score = prev->branches[i]->score + 1;
//      (*(prev->branches+i))->score++;
      return;
    }
  }
  prev->branches[prev->branch_list_index] = build_branch(cur);
//  *(prev->branches+prev->branch_list_index) = build_branch(cur);
  prev->branch_list_index = prev->branch_list_index + 1;

  
  print_branches(prev);
  //(*(prev->branches) + prev->branch_list_index) = build_branch(cur);
}
struct word * build_word(char * in, int len) {
  debug("build_Word %s, %d", in, len);
  char * word_value = (char *) malloc(len+1);
  memset(word_value, 0, len+1);
  memcpy(word_value, in, len);
  //word_value[len+1] = (char) 0x00;
  //*(word_value+len+1) = (char) 0x00;
  
  struct word * new_word = (struct word *) malloc(sizeof(struct word));
  memset(new_word,0,sizeof(struct word));
  new_word->value = (char *) word_value;

  new_word->len = len;
  new_word->max_branches = starting_branch_size;
  new_word->branch_list_index = 0;
  new_word->branches = (struct branch **) malloc(starting_branch_size*sizeof(struct branch *));
  memset(new_word->branches, 0, starting_branch_size*sizeof(struct branch *));
  return new_word;
}

void prep_branch_list(struct word * cur_word) {
  debug("prep_branch_list %p",cur_word);
 /* if (cur_word->max_branches == 0) { // could be more efficient if this is included in build_word
    cur_word->branches = (struct branch **) malloc(starting_branch_size*sizeof(struct branch *));
    memset(cur_word->branches,0,starting_branch_size*sizeof(struct branch *));

    cur_word->max_branches = starting_branch_size;
   // cur_word->branches = branches;
  
  } else */ if(cur_word->branch_list_index>=cur_word->max_branches) {
    //struct branch ** branches = (struct branch **) malloc(cur_word->max_branches * 2 * sizeof(struct branch *));
    //memset(branches, 0, cur_word->max_branches * 2 * sizeof(struct branch *));
    //memcpy(branches, cur_word->branches,cur_word->max_branches*sizeof(struct branch *));

    cur_word->max_branches = cur_word->max_branches * 2;
    //free(cur_word->branches);
    cur_word->branches = (struct branch **) realloc(cur_word->branches, cur_word->max_branches*sizeof(struct branch *));
    //cur_word->branches = branches;
  }
}
struct word_node * build_node(struct word * item) {
  debug("build_node: %p",item);
  struct word_node * this_node = (struct word_node *) malloc(sizeof(struct word_node));
  memset(this_node, 0, sizeof(struct word_node));
  this_node->left = NULL;
  this_node->right = NULL;
  this_node->item = item;
  return this_node;
}

struct word_node * insert_str_as_node(struct word ** new_word_ptr, struct word_node * tree_item, char * buff, int buff_len) {
  debug("insert_str_as_node: %p,%p",new_word, tree_item);
  if(tree_item == NULL) {
    new_word_ptr[0] = build_word(buff,buff_len);
    tree_item = build_node(new_word_ptr[0]);
  } else {
    int cmp_res = strncmp(tree_item->item->value,buff,max(buff_len,tree_item->item->len));
    if(cmp_res ==0) {
      new_word_ptr[0] = tree_item->item;
    //  if(use_weights==1) {
     // struct word * temp = tree_item->item;
      //memcpy(new_word,tree_item->item,sizeof(struct word));
      //tree_item->item = new_word;
      //  }
        /*
        struct word * temp = new_word;
        printf("-------------------\n");
        printf("initially \t\t\tnew_word is: %p, temp is: %p, tree_item->item is: %p\n",new_word, temp, tree_item->item);
        new_word = tree_item->item;
        printf("after updating item record\tnew_word is: %p, temp is: %p, tree_item->item is: %p\n",new_word, temp, tree_item->item);
        free(temp);
        printf("after free temp \t\tnew_word is: %p, temp is: %p, tree_item->item is: %p\n",new_word, temp, tree_item->item);
        printf("-------------------\n");
      // SAME WORD ALREADY HERE. merge them!*/
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
