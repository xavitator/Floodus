#ifndef _IOVEC_H
#define _IOVEC_H

#include <sys/uio.h>

void freeiovec(struct iovec *data);
struct iovec *copy_iovec(struct iovec *data);
int compare_iovec(struct iovec *data1, struct iovec *data2);
void print_iovec(struct iovec *data);

#endif