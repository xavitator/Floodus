#ifndef H_DEBUG
#define H_DEBUG


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "view.h"

#define DEBUG 1

#define false 0
#define true 1

typedef u_int8_t bool_t;


void debug(uint8_t flag, uint8_t error, char *name, const char *msg);
void debug_and_exit(uint8_t flag, uint8_t error, char *name, const char *msg, int exit_code);
void debug_hex(uint8_t flag, uint8_t error, char *name, void *data, int data_len);
void debug_hex_and_exit(uint8_t flag, uint8_t error, char *name, void *data, int data_len, int exit_code);
void debug_int(uint8_t flag, uint8_t error, char *name, int rc);
void debug_int_and_exit(uint8_t flag, uint8_t error, char *name, int rc, int exit_code);

#endif
