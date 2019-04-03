#ifndef _INONDATION_H
#define _INONDATION_H

#define D_INOND 1

#include <time.h>

#include "debug.h"
#include "hashmap.h"
#include "voisin.h"

typedef struct message_t
{
    struct timespec send_time; // temps absolu à partir duquel on peut envoyer le message
    u_int8_t count;            // compteur du nombre d'envoi effectué
    u_int8_t type;             // type du message
    char *content;             // tableau de char contenant le message à transmettre
    u_int8_t contentlen;       // taille du champs 'content'
    u_int64_t id;              // id de celui qui a envoyé le message
    u_int32_t nonce;           // nounce rendant le message unique
    hashmap_t *recipient;      // ensemble de ceux à qui on doit envoyer le message sous forme (ip_port_t, ip_port_t)
    struct message_t *next;    // message suivant dans la liste
} message_t;

#endif