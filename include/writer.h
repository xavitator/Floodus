#include "hashmap.h"
#include "voisin.h"

#define D_WRITER 1

#define MAX_PER_TLV 2048

extern u_int32_t g_socket;

void init_writer(u_int32_t socket);
bool_t send_tlv(ip_port_t *ipport, data_t *database, size_t len);