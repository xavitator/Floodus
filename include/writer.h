#ifndef _WRITER_H
#define _WRITER_H

#include "hashmap.h"
#include "voisin.h"
#include "controller.h"

#define D_WRITER 1

#define MAX_PER_TLV 1000

void clear_all();
bool_t send_tlv(ip_port_t *ipport, data_t *database, size_t len);
bool_t add_tlv(ip_port_t dest, data_t *tlv);
bool_t buffer_is_empty(void);
bool_t send_buffer_tlv(void);

#endif