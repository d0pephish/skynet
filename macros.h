
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
  #ifdef NDEBUGFP
  FILE * DEBUG_fp;
  #define debug(M, ...) DEBUG_fp = fopen("debug.output","a");fprintf(DEBUG_fp, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__); fclose(DEBUG_fp)
  #else
  #define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
  #endif
#endif

