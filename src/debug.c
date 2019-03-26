#include "debug.h"

void debug (char * name, const char *msg) {
  if (DEBUG) {
    fprintf(stderr, "[DEBUG] Str %s : %s\n",name, msg);
  }
}

void debug_and_exit(char * name, const char *msg, int exit_code) {
  if (DEBUG) {
    fprintf(stderr, "[DEBUG] Str %s : %s\n",name, msg);
    exit(exit_code);
  }
}

void debug_hex(char *name, uint8_t *data, int length) {
  if (DEBUG) {
    fprintf(stderr, "[DEBUG] Hexa %s : ", name);
    for (int i = 0 ; i < length ; i++) {
      fprintf(stderr, "%.2x ", data[i]);
    } 
    fprintf(stderr, "\n");
  }
}

void debug_hex_and_exit (char *name, uint8_t *data, int length, int exit_code) {
  if (DEBUG) {
    fprintf(stderr, "[DEBUG] Hexa %s : ", name);
    for (int i = 0 ; i < length ; i++) {
      fprintf(stderr, "%.2x ", data[i]);
    } 
    fprintf(stderr, "\n");
    exit(exit_code);
  }
}

void debug_int(char *name, int rc) {
  if(DEBUG) {
    fprintf(stderr, "[DEBUG] Int %s: %d\n",name, rc);
  }
}

void debug_int_and_exit(char *name, int rc, int exit_code) {
  if(DEBUG) {
    fprintf(stderr, "[DEBUG] Int %s : %d\n",name, rc);
    exit(exit_code);
  }
}


