#include "TLV.h"

/**
 * @brief Créer une struct iovec pour PDA1
 */
struct iovec *pda1() {
  struct iovec *pad = malloc(sizeof(struct iovec));
  uint8_t content [1] = {0};
  memset(&content,0,sizeof(content));
  pad->iov_base = content;
  pad->iov_len = sizeof(content);
  return pad;
}


/**
 * @brief Créer une struct iovec pour PDAN
 *
 * @param La taille du N
 */
struct iovec *pda_n(int N) {
  struct iovec *pad = malloc(sizeof(struct iovec));
  uint8_t content [N+2];
  memset(&content,0,sizeof(content));
  content[0] = 1;
  content[1] = N;
  pad->iov_base = content;
  pad->iov_len = sizeof(content);
  return pad;
}

/**
 * @brief Construit un TLV Hello Court
 *
 * @param source_id l'id de 64 bits de l'émetteur
 */
struct iovec *hello_short(uint8_t source_id[8]) {
  struct iovec *hello = malloc(sizeof(struct iovec));
  uint8_t content [8+2];
  memset(&content,0,sizeof(content));
  content[0] = 2;
  content[1] = 8;
  memcpy(content+2,source_id,8);
  hello->iov_base = content;
  hello->iov_len = sizeof(content);
  return hello;
}

/**
 * @brief Construit un TLV Hello Long
 *
 * @param source_id l'id de 64 bits de l'émetteur
 * @param id l'id de 64 bits du destinataire
 */
struct iovec *hello_long(uint8_t source_id[8], uint8_t id[8]) {
  struct iovec *hello = malloc(sizeof(struct iovec));
  uint8_t content [16+2];
  memset(&content,0,sizeof(content));
  content[0] = 2;
  content[1] = 16;
  memcpy(content+2,source_id,8);
  memcpy(content+10,id, 8);
  hello->iov_base = content;
  hello->iov_len = sizeof(content);
  return hello;
}
