#ifndef _INONDATION_H
#define _INONDATION_H

#define D_INOND 1

#define COUNT_INOND 6

#include <time.h>
#include <stdio.h>
#include <math.h>

#include "debug.h"
#include "TLV.h"
#include "hashmap.h"
#include "voisin.h"

typedef struct message_t
{
    struct timespec send_time; // temps absolu à partir duquel on peut envoyer le message
    u_int8_t count;            // compteur du nombre d'envoi effectué
    u_int8_t type;             // type du message
    u_int8_t *content;         // tableau de char contenant le message à transmettre
    u_int8_t contentlen;       // taille du champs 'content'
    u_int64_t id;              // id de celui qui a envoyé le message
    u_int32_t nonce;           // nounce rendant le message unique
    hashmap_t *recipient;      // ensemble de ceux à qui on doit envoyer le message sous forme (ip_port_t, ip_port_t)
    struct message_t *next;    // message suivant dans la liste
} message_t;

void free_inondation();
int compare_time(struct timespec ta, struct timespec tb);
bool_t add_message(ip_port_t sender, u_int64_t id, uint32_t nonce, uint8_t type, char *content, u_int8_t contentlen);
bool_t get_nexttime(struct timespec *tm);
bool_t launch_flood();

#endif