typedef struct {
  connection * c;
  char * server_name;
  char * bot_name;
  char * log_name;
  char * settings_filename;
  FILE * log_fp;
  pthread_t * lth;
  int server_port;
} irc_session_node;

