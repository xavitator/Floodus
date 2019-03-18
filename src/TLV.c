#include "TLV.h"


/* Fonction affichage erreur */
static void error(char * obj) {
  fprintf (stderr,"Erreur TLV => can't allocate %s\n", obj);
} 


/**
 * @brief Créer une struct iovec pour PDA1
 */
struct iovec *pad1() {
  struct iovec *pad = malloc(sizeof(struct iovec));
  if (pad == NULL) {
    error("iovec pad");
    return NULL;
  }
  uint8_t size = 1;
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL) {
    error("content pad");
    free(pad);
    return NULL;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  pad->iov_base = content;
  pad->iov_len = size;
  return pad;
}


/**
 * @brief Créer une struct iovec pour PDAN
 *
 * @param La taille du N
 */
struct iovec *pad_n(uint8_t N) {
  struct iovec *pad = malloc(sizeof(struct iovec));
  if (pad == NULL) {
    error("iovec pad_n");
    return NULL;
  }
  uint8_t size = ((N < 253) ? N : N%253) +2;
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL) {
    error("content pad_n");
    free(pad);
    return NULL;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  content[0] = 1;
  content[1] = N;
  pad->iov_base = content;
  pad->iov_len = size;
  return pad;
}

/**
 * @brief Construit un TLV Hello Court
 *
 * @param source_id l'id de 64 bits de l'émetteur
 */
struct iovec *hello_short(uint64_t sender_id) {
  struct iovec *hello = malloc(sizeof(struct iovec));
  if (hello == NULL) {
    error("iovec hello");
    return NULL;
  }
  uint8_t size = 10;
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL) {
    error("content hello short");
    free(hello);
    return NULL;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  content[0] = 2;
  content[1] = 8;
  memmove(content+2, &sender_id, sizeof(uint64_t));
  hello->iov_base = content;
  hello->iov_len = size;
  return hello;
}

/**
 * @brief Construit un TLV Hello Long
 *
 * @param source_id l'id de 64 bits de l'émetteur
 * @param id l'id de 64 bits du destinataire
 */
struct iovec *hello_long(uint64_t sender_id, uint64_t id) {
  struct iovec *hello = malloc(sizeof(struct iovec));
  if (hello == NULL) {
    error("iovec hello_long");
    return NULL;
  }
  uint8_t size = 18;
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL) {
    error("content hello long");
    free(hello);
    return NULL;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  content[0] = 2;
  content[1] = 16;
  memmove(content+2, &sender_id, sizeof(uint64_t));
  memmove(content+10, &id, sizeof(uint64_t));
  hello->iov_base = content;
  hello->iov_len = size;
  return hello;
}


/**
 * @brief Construit un Neighbour
 *
 * @param ip l'ip à partager
 * @param port le port de l'ip
 */
struct iovec *neighbour(uint8_t source_ip[16], uint16_t port) {
  struct iovec *neighbour_i = malloc(sizeof(struct iovec));
  if (neighbour_i == NULL) {
    error("iovec neighbour");
    return NULL;
  }
  uint8_t size = 20;
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL) {
    error("content neighbour");
    free(neighbour_i);
    return NULL;
  }
  memset(content, 0, 20 * sizeof(uint8_t));
  content[0] = 3;
  content[1] = 18;
  memmove(content+2, source_ip, sizeof(uint8_t)* 16);
  memmove(content+16, &port, sizeof(uint16_t));
  neighbour_i->iov_base = content;
  neighbour_i->iov_len = size;
  return neighbour_i;
}


/**
 * @brief Construit un acquitement pour une data
 *
 * @param sender_id l'id de  l'émetteur
 * @param nonce l'apax pour l'identification
 * @param type le type de message (0)
 * @param msg_length la taille du message
 * @param msg le message à envoyer
 */
struct iovec *data(uint64_t sender_id, uint32_t nonce, uint8_t type, uint32_t msg_length, uint8_t *msg) {
  struct iovec *data_i = malloc(sizeof(struct iovec));
  if (data_i == NULL) {
    error("iovec ack");
    return NULL;
  }
  uint8_t size =  (15 + msg_length);
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL) {
    error("content neighbour");
    free(data_i);
    return NULL;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  content[0] = 5;
  content[1] = 12;
  memmove(content+2, &sender_id, sizeof(uint64_t));
  memmove(content+10, &nonce, sizeof(uint32_t));
  content [15] = type;
  memmove(content+10, msg, msg_length);
  data_i->iov_base = content;
  data_i->iov_len = size;
  return data_i;
}



/**
 * @brief Construit un acquitement pour une data
 *
 * @param sender_id l'id de l'envoyeur (copie)
 * @param nonce l'apax pour l'acquitement (copie)
 */
struct iovec *ack(uint64_t sender_id, uint32_t nonce) {
  struct iovec *ack_i = malloc(sizeof(struct iovec));
  if (ack_i == NULL) {
    error("iovec ack");
    return NULL;
  }
  uint32_t size = 14;
  uint8_t *content  = malloc(sizeof(uint8_t) * size);
  if (content == NULL) {
    error("content ack");
    free(ack_i);
    return NULL;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  content[0] = 5;
  content[1] = 12;
  memmove(content+2, &sender_id, sizeof(uint64_t));
  memmove(content+10, &nonce, sizeof(uint32_t));
  ack_i->iov_base = content;
  ack_i->iov_len = size;
  return ack_i;
}


/**
 * @brief Construit un TLV go away
 *
 * @param code la raison du go away
 * @param msg_length la taille du message
 * @param msg le message à joindre
 */
struct iovec *go_away(uint8_t code, uint32_t msg_length, uint8_t *msg) {
  struct iovec *go_away_i = malloc(sizeof(struct iovec));
  if (go_away_i == NULL) {
    error("iovec go_away");
    return NULL;
  }
  uint8_t size = 3 + msg_length;
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL) {
    error("content go_away");
    free(go_away_i);
    return NULL;
  }
  memset(&content, 0, sizeof(uint8_t) * size);
  content[0] = 6;
  content[1] = msg_length+1;
  content[2] = code;
  memmove(content+3, msg, msg_length);
  go_away_i->iov_base = content;
  go_away_i->iov_len = size;
  return go_away_i;
}


/**
 * @brief Construit un TLV de warning
 *
 * @param msg_length la taille du message
 * @param msg le message
 */
struct iovec *warning(uint32_t msg_length, uint8_t *msg) {
  struct iovec *warning_i = malloc(sizeof(struct iovec));
  if (warning_i == NULL) {
    error("iovec warning");
    return NULL;
  }
  uint8_t size = 2 + msg_length;
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL) {
    error("content warning");
    free(warning_i);
    return NULL;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  content[0] = 7;
  content[1] = msg_length+1;
  memmove(content+2, msg, msg_length);
  warning_i->iov_base = content;
  warning_i->iov_len = size;
  return warning_i;
}
