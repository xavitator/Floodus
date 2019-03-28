#ifndef _IOVEC_H
#define _IOVEC_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>

#include "debug.h"

#define D_IOVEC 0

typedef struct iovec data_t;

struct iovec *create_iovec(void *data, size_t len);
void freeiovec(struct iovec *data);
struct iovec *copy_iovec(struct iovec *data);
int compare_iovec(struct iovec *data1, struct iovec *data2);
void print_iovec(struct iovec *data);

#endif