#ifndef TLV_H
#define TLV_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <string.h>

#include "debug.h"

#define D_TLV 0

struct iovec *pad1();
struct iovec *pad_n(uint8_t N);
struct iovec *hello_short(uint64_t source_id);
struct iovec *hello_long(uint64_t source_id, uint64_t id);
struct iovec *neighbour(uint8_t source_ip[16], uint16_t port);
struct iovec *data(uint64_t sender_id, uint32_t nonce, uint8_t type, uint8_t msg_length, uint8_t *msg);
struct iovec *ack(uint64_t sender_id, uint32_t nonce);
struct iovec *go_away(uint8_t code, uint8_t msg_length, uint8_t *msg);
struct iovec *warning(uint8_t msg_length, uint8_t *msg);

#endif
