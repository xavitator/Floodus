#ifndef _VOISIN_H
#define _VOISIN_H

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "iovec.h"
#include "hashmap.h"
#include "TLV.h"

#define D_VOISIN 1

/**
 * @brief Structure contenant l'ip et le port d'un individu.
 * Le champs 'ipv6' correspond à l'ipv6 de l'individu.
 * Le champs 'port' correspond au port via lequel l'individu se connecte au réseau.
 */
typedef struct ip_port_t
{
    u_int8_t ipv6[IPV6_LEN];
    u_int16_t port;
} ip_port_t;

/**
 * @brief Structure contenant toutes les informations sur un voisin.
 * Le champs 'id' correspond à l'id de l'individu.
 * Le champs 'hello' correspond au temps auquel on a reçu le dernier hello.
 * le champs 'long_hello' correspond au temps auquel on a reçu le dernier hello long.
 */
typedef struct neighbour_t
{
    u_int64_t id;
    struct timespec hello;
    struct timespec long_hello;
} neighbour_t;

#include "writer.h"

extern u_int64_t g_myid;
extern hashmap_t *g_neighbours;
extern hashmap_t *g_environs;
extern pthread_mutex_t g_lock_n;
extern pthread_mutex_t g_lock_e;

short lock(pthread_mutex_t *);
short unlock(pthread_mutex_t *);
void create_user(void);
bool_t init_neighbours(void);
bool_t update_neighbours(node_t *node, char *msg); 
void free_neighbors(void);
bool_t apply_tlv_hello(ip_port_t src, data_t *data, size_t *head_read);
bool_t apply_tlv_neighbour(data_t *data, size_t *head_read);
bool_t apply_tlv_goaway(ip_port_t src, data_t *data, size_t *head_read); 
bool_t is_symetric(ip_port_t ipport);
bool_t is_more_than_two(struct timespec tm);

#endif
