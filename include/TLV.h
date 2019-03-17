#ifndef TLV_H
#define TLV_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <string.h>

struct iovec * pda1();
struct iovec * pda_n(int N);
struct iovec * hello_short(uint8_t source_id[8]);
struct iovec * hello_long(uint8_t source_id[8], uint8_t id[8]);

#endif
