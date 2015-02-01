#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
// MACROS
#ifndef max
  #define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
  #define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif


//SETTINGS
int random_choice = 0;
int starting_branch_size = 4;
//STRUCTS
struct branch {
  int score;
  struct word * item;
};//TO DO: add branch functionality
struct word {
  char * value;
  int len;
  struct branch ** branches;
  int id;
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
struct word_node * insert_node(struct word *, struct word_node *);
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
        printf("parsing: %s\n",buf);
        parse(buf);
        count = 0;
        buf_p = (char *) &buf;
        memset(buf_p, 0, 1024);
      }
    }
  }
  srand(time(NULL));
  build_sentence("i");
  build_sentence(gen_random_word_from_tree());
}

void parse(char * input) {
  char * in_ptr = input;
  char buff[250];
  char * buff_ptr = (char *) &buff;
  struct word * prev_word = NULL;
  struct word * cur_word = NULL;
  int word_size = 0;

  memset(buff_ptr, 0, 250);
  while ((*in_ptr)!=0x00) {

    if (!valid_char((char) *in_ptr)) {
  
    //  printf("invalid char! skipping...\n");
  
    } else if(word_size >=249) {
      
      printf("word too big! stopping...\n");
      break;
    
    } else if (*in_ptr == ' ' || *in_ptr == '.') {
      
      if (*in_ptr == '.') {
        prev_word =NULL;
   //     printf(".\n"); 
      
      }
      
      if (word_size>0) {
        
  //      printf("%s\n",buff);
        cur_word = build_word(buff, word_size);
        word_list = insert_node(cur_word,word_list);
        if(prev_word != NULL) link_words(prev_word,cur_word);
      
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
  printf("Building sentence from '%s':\n%s",input,input);
  struct word * starting_word = find_word(input,word_list); 
  if(starting_word == NULL) {
    printf("no links to follow with this input. quitting\n");
    return;
  }
  walk_link(starting_word);
}
void walk_link(struct word * item) {
  if(item==NULL) return; 
  struct branch * best_branch;
  struct branch * cur_branch;
  int best_score = INT_MIN;
  int i;
  for(i=0;i<item->branch_list_index;i++) {
    cur_branch = *(item->branches+i);
      if(cur_branch==NULL) return;
      if(cur_branch->item==NULL) printf("SOMETHING WICKED HAPPENED HERE ! \n");
    if(cur_branch->score > best_score) {
      best_score = cur_branch->score;
      best_branch = cur_branch;
    } else if(cur_branch->score == best_score) { 
      if(rand()%2 == 1) {
        best_branch = cur_branch;
      }
    }
  }
  if(best_score <= -1) {
    printf(".\n");
    return;
  }
  printf(" %s",best_branch->item->value);
  best_branch->score--;
  walk_link(best_branch->item);
}
struct word * find_word(char * input, struct word_node * item) {
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
  int i,cmp;
  struct word * branch_list_word;
  if(prev==NULL) return;
  prep_branch_list(prev);
  //check if word is already in a branch, if not, add it. 
  for(i = 0; i<prev->branch_list_index; i++) {
    branch_list_word = (*(prev->branches+i))->item;
    cmp = strncmp(branch_list_word->value, cur->value, max(branch_list_word->len, cur->len));
    if(cmp==0) {
      (*(prev->branches+i))->score++;
      return;
    }
  }
  *(prev->branches+prev->branch_list_index) = build_branch(cur);
  prev->branch_list_index++;
  //(*(prev->branches) + prev->branch_list_index) = build_branch(cur);
}
struct word * build_word(char * in, int len) {
  char * word_value = (char *) malloc(len+1);
  memcpy(word_value, in, len);
  *(word_value+len+1) = (char) 0x00;
  
  struct word * new_word = malloc(sizeof(struct word));
  memset(new_word,0,sizeof(struct word));
  new_word->value = word_value;

  new_word->len = len;
  new_word->max_branches = 0;
  new_word->branch_list_index = 0;
  
  return new_word;
}

void prep_branch_list(struct word * cur_word) {
  if (cur_word->max_branches == 0) { // could be more efficient if this is included in build_word
    struct branch ** branches = (struct branch **) malloc(starting_branch_size*sizeof(struct branch *));
    memset(branches,0,starting_branch_size*sizeof(struct branch *));

    cur_word->max_branches = starting_branch_size;
    cur_word->branches = branches;
  
  } else if(cur_word->branch_list_index>=cur_word->max_branches) {
    struct branch ** branches = (struct branch **) malloc(cur_word->max_branches * 2 * sizeof(struct branch *));
    memset(branches, 0, cur_word->max_branches * 2 * sizeof(struct branch *));
    memcpy(branches, cur_word->branches,cur_word->max_branches*sizeof(struct branch *));

    cur_word->max_branches = cur_word->max_branches * 2;
    //free(cur_word->branches);
    cur_word->branches = branches;
  }
}
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
      struct word * temp = tree_item->item;
      memcpy(new_word,tree_item->item,sizeof(struct word));
      tree_item->item = new_word;
      // SAME WORD ALREADY HERE. merge them!
    } else if(cmp_res >0) {
      tree_item->left = insert_node(new_word, tree_item->left);
    } else {
      tree_item->right = insert_node(new_word, tree_item->right);
    }
    
  }
  return tree_item;
}
char * gen_random_word_from_tree() {
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
  if (*i<=1) return tree_node->item;
  if (tree_node== NULL) return NULL;
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
  printf("%s (%d):",tree_node->item->value,tree_node->item->branch_list_index);
  print_branches(tree_node->item);
  printf("\n");
  print_tree(tree_node->right);
}
void print_branches(struct word * item) {
  int i;
  for(i=0; i<item->branch_list_index; i++) {
    printf("->%s,%d;",(*(item->branches+i))->item->value, (*(item->branches+i))->score);
  }
}
