/**
 * @file TLV.c
 * @author Floodus
 * @brief Module s'occupant de tout ce qui est constructon de TLV
 * 
 */

#include "TLV.h"

/* Fonction affichage erreur */
static void error(char *obj)
{
  debug(D_TLV, 1, obj, "Erreur TLV => can't allocate memory");
}

/**
 * @brief Créer une struct iovec pour PDA1
 * 
 * @param pad struct iovec à remplir
 * @return bool_t '1' si le remplissage s'est bien passé, '0' sinon.
 */
bool_t pad1(data_t *pad)
{
  if (pad == NULL)
  {
    error("iovec pad");
    return false;
  }
  uint8_t size = 1;
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL)
  {
    error("content pad");
    free(pad);
    return false;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  pad->iov_base = content;
  pad->iov_len = size;
  debug_hex(D_TLV, 0, "pad1 contruction TLV", pad->iov_base, pad->iov_len);
  return true;
}

/**
 * @brief Créer une struct iovec pour PDAN
 *
 * @param pad struct iovec à remplir
 * @param N La taille dans le PADN, en cas de dépassement 253
 * @return bool_t '1' si le remplissage s'est bien passé, '0' sinon.
 */
bool_t pad_n(data_t *pad, uint8_t N)
{
  if (pad == NULL)
  {
    error("iovec pad_n");
    return false;
  }
  uint8_t size = ((253 - N > 0) ? N : 253) + 2;
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL)
  {
    error("content pad_n");
    free(pad);
    return false;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  content[0] = 1;
  content[1] = N;
  pad->iov_base = content;
  pad->iov_len = size;
  debug_hex(D_TLV, 0, "pad_n contruction TLV", pad->iov_base, pad->iov_len);
  return true;
}

/**
 * @brief Construit un TLV Hello Court
 *
 * @param hello struct iovec à remplir
 * @param sender_id l'id de 64 bits de l'émetteur 
 * @return bool_t '1' si le remplissage s'est bien passé, '0' sinon.
 */
bool_t hello_short(data_t *hello, uint64_t sender_id)
{
  if (hello == NULL)
  {
    error("iovec hello_short");
    return false;
  }
  uint8_t size = 10;
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL)
  {
    error("content hello_short");
    free(hello);
    return false;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  content[0] = 2;
  content[1] = 8;
  memmove(content + 2, &sender_id, sizeof(uint64_t));
  hello->iov_base = content;
  hello->iov_len = size;
  debug_hex(D_TLV, 0, "hello_short contruction TLV", hello->iov_base, hello->iov_len);
  return true;
}

/**
 * @brief Construit un TLV Hello Long
 *
 * @param hello struct iovec à remplir
 * @param sender_id l'id de 64 bits de l'émetteur
 * @param id l'id de 64 bits du destinataire
 * @return bool_t '1' si le remplissage s'est bien passé, '0' sinon.
 */
bool_t hello_long(data_t *hello, uint64_t sender_id, uint64_t id)
{
  if (hello == NULL)
  {
    error("iovec hello_long");
    return false;
  }
  uint8_t size = 18;
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL)
  {
    error("content hello_long");
    free(hello);
    return false;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  content[0] = 2;
  content[1] = 16;
  memmove(content + 2, &sender_id, sizeof(uint64_t));
  memmove(content + 10, &id, sizeof(uint64_t));
  hello->iov_base = content;
  hello->iov_len = size;
  debug_hex(D_TLV, 0, "hello_long contruction TLV", hello->iov_base, hello->iov_len);
  return true;
}

/**
 * @brief Construit un Neighbour
 *
 * @param neighbour_i struct iovec à remplir
 * @param source_ip l'ip à partager
 * @param port le port de l'ip
 * @return bool_t '1' si le remplissage s'est bien passé, '0' sinon.
 */
bool_t neighbour(data_t *neighbour_i, uint8_t source_ip[16], uint16_t port)
{
  if (neighbour_i == NULL)
  {
    error("iovec neighbour");
    return false;
  }
  uint8_t size = 20;
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL)
  {
    error("content neighbour");
    free(neighbour_i);
    return false;
  }
  memset(content, 0, 20 * sizeof(uint8_t));
  content[0] = 3;
  content[1] = 18;
  memmove(content + 2, source_ip, sizeof(uint8_t) * 16);
  memmove(content + 18, &port, sizeof(uint16_t));
  neighbour_i->iov_base = content;
  neighbour_i->iov_len = size;
  debug_hex(D_TLV, 0, "neighbour contruction TLV", neighbour_i->iov_base, neighbour_i->iov_len);
  return true;
}

/**
 * @brief Construit un acquitement pour une data
 *
 * @param data_i struct iovec à remplir
 * @param sender_id l'id de  l'émetteur
 * @param nonce l'apax pour l'identification
 * @param type le type de message (0)
 * @param msg_length la taille du message, prend le min avec 240
 * @param msg le message à envoyer
 * @return bool_t '1' si le remplissage s'est bien passé, '0' sinon.
 */
bool_t data(data_t *data_i, uint64_t sender_id, uint32_t nonce,
            uint8_t type, uint8_t *msg, uint8_t msg_length)
{
  if (data_i == NULL)
  {
    error("iovec data");
    return false;
  }
  uint8_t size = 15 + ((240 - msg_length > 0) ? msg_length : 240);
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL)
  {
    error("content data");
    free(data_i);
    return false;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  content[0] = 5;
  content[1] = 12;
  memmove(content + 2, &sender_id, sizeof(uint64_t));
  memmove(content + 10, &nonce, sizeof(uint32_t));
  content[15] = type;
  memmove(content + 10, msg, msg_length);
  data_i->iov_base = content;
  data_i->iov_len = size;
  debug_hex(D_TLV, 0, "data contruction TLV", data_i->iov_base, data_i->iov_len);
  return true;
}

/**
 * @brief Construit un acquitement pour une data
 *
 * @param ack_i struct iovec à remplir
 * @param sender_id l'id de l'envoyeur (copie)
 * @param nonce l'apax pour l'acquitement (copie)
 * @return bool_t '1' si le remplissage s'est bien passé, '0' sinon.
 */
bool_t ack(data_t *ack_i, uint64_t sender_id, uint32_t nonce)
{
  if (ack_i == NULL)
  {
    error("iovec ack");
    return false;
  }
  uint32_t size = 14;
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL)
  {
    error("content ack");
    free(ack_i);
    return false;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  content[0] = 5;
  content[1] = 12;
  memmove(content + 2, &sender_id, sizeof(uint64_t));
  memmove(content + 10, &nonce, sizeof(uint32_t));
  ack_i->iov_base = content;
  ack_i->iov_len = size;
  debug_hex(D_TLV, 0, "ack contruction TLV", ack_i->iov_base, ack_i->iov_len);
  return true;
}

/**
 * @brief Construit un TLV go away
 *
 * @param go_away_i struct iovec à remplir
 * @param code la raison du go away
 * @param msg_length la taille du message, prend le min entre msg_length et 252
 * @param msg le message à joindre
 * @return bool_t '1' si le remplissage s'est bien passé, '0' sinon.
 */
bool_t go_away(data_t *go_away_i, uint8_t code, uint8_t *msg, uint8_t msg_length)
{
  if (go_away_i == NULL)
  {
    error("iovec go_away");
    return false;
  }
  uint8_t size = 3 + ((252 - msg_length > 0) ? msg_length : 252);
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL)
  {
    error("content go_away");
    free(go_away_i);
    return false;
  }
  memset(&content, 0, sizeof(uint8_t) * size);
  content[0] = 6;
  content[1] = msg_length + 1;
  content[2] = code;
  memmove(content + 3, msg, msg_length);
  go_away_i->iov_base = content;
  go_away_i->iov_len = size;
  debug_hex(D_TLV, 0, "go_away contruction TLV", go_away_i->iov_base, go_away_i->iov_len);
  return true;
}

/**
 * @brief Construit un TLV de warning
 *
 * @param warning_i struct iovec à remplir
 * @param msg_length la taille du message, prend le min entre msg_length et 253
 * @param msg le message
 * @return bool_t '1' si le remplissage s'est bien passé, '0' sinon.
 */
bool_t warning(data_t *warning_i, uint8_t *msg, uint8_t msg_length)
{
  if (warning_i == NULL)
  {
    error("iovec warning");
    return false;
  }
  uint8_t size = 2 + ((253 - msg_length > 0) ? msg_length : 253);
  uint8_t *content = malloc(sizeof(uint8_t) * size);
  if (content == NULL)
  {
    error("content warning");
    free(warning_i);
    return false;
  }
  memset(content, 0, sizeof(uint8_t) * size);
  content[0] = 7;
  content[1] = msg_length + 1;
  memmove(content + 2, msg, msg_length);
  warning_i->iov_base = content;
  warning_i->iov_len = size;
  debug_hex(D_TLV, 0, "warning contruction TLV", warning_i->iov_base, warning_i->iov_len);
  return true;
}
