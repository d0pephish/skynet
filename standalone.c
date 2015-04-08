#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "parser_library.c"
/*

  This is version 0.1 of a predictive text tool. Every link between one word and the next is tracked, and multiple occurances of a link increases its score. 
  
  There are two phases to this tool: parsing and generation. 
    Parsing: every sentence run throgh parse() function is added to the database. You can call the function multiple times and it will affect the same tree.
    Generation: Give a one-word seed to build_sentence(). It will find the word in the list and walk its chain, producing a sentence. There is a function that will generate a random word for you, however it is really clunky and inefficient for now.
    CHANGELOG
    v0.1 supports parsing and tracking first order links between words. 




*/

//BEGIN



int main(int argc, char * argv[]) {
  init("",0);
  FILE *fp;
  char * word_seed = NULL;
  char * output;
  if(argc<=1) {
  printf("must give file to read from: %s <filename>. Running on sample data.\n",argv[0]);
  parse("A comparative statement has the format if then. this is a test of the predictive text. If it works, this might tell you a pattern of what words to use. We'll havee to see if it works. I wonder if it will tell you what word comes next in the sentence or if it will be way off in its predictions. the more data there is, the better it will be. ");
  } else {
    fp = fopen(argv[1], "r");
    parse_from_file(fp);
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
    if(argc==3) {
      
      output = build_sentence(argv[2], word_list);
      printf("%s",output);
      free(output);
      return 0;
    }
  }
  printf("interactive mode? (y/n): ");
  if(getchar() == 'y') {
    printf("beginning interactive mode. press Ctrl+C to quit.\n");

    while(1) {
      word_seed = get_user_input("Enter a single word: ");
      output = build_sentence(word_seed, word_list);
      printf("%s",output);
      free(output);
      free(word_seed);
    }
  } else {
    srand(time(NULL));
    output = build_sentence(gen_random_word_from_tree(),word_list);
    printf("%s",output);
    free(output);
  }
  destroy_tree(word_list);
  return 0;
}
