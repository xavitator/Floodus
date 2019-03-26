#ifndef H_DEBUG
#define H_DEBUG

#define DEBUG 1 

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void debug(char *name, const char *msg);
void debug_and_exit(char *name, const char *msg, int exit_code);
void debug_hex(char *name, uint8_t *data,int length);
void debug_hex_and_exit(char *name, uint8_t *data,int length, int exit_code);
void debug_int(char *name, int rc);
void debug_int_and_exit(char *name, int rc, int exit_code);

#endif
