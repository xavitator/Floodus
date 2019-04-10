#ifndef TLV_H
#define TLV_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <string.h>

#include "debug.h"
#include "iovec.h"

#define D_TLV 0
#define IPV6_LEN 16

data_t * pad1(void);
data_t * pad_n(uint8_t len);
data_t * hello_short(uint64_t dest_id);
data_t * hello_long(uint64_t dest_id, uint64_t src_id);
data_t * neighbour(uint8_t src_ip[IPV6_LEN], uint16_t port);
data_t * data(uint64_t dest_id, uint32_t nonce, uint8_t type, uint8_t *msg, uint8_t msg_length);
data_t * ack(uint64_t dest_id, uint32_t nonce);
data_t * go_away(uint8_t code, uint8_t *msg, uint8_t msg_length);
data_t * warning(uint8_t *msg, uint8_t msg_length);

#endif
