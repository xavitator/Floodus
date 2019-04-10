#ifndef _WRITER_H
#define _WRITER_H

#include "hashmap.h"
#include "voisin.h"
#include "controller.h"

#define D_WRITER 1

#define MAX_PER_TLV 1000

extern u_int32_t g_socket;
extern pthread_mutex_t g_lock_buff;

void clear_all(void);
bool_t send_tlv(ip_port_t dest, data_t *tlvs, size_t tlvs_len);
bool_t add_tlv(ip_port_t dest, data_t *tlv);
bool_t buffer_is_empty(void);
bool_t send_buffer_tlv(void);

#endif
