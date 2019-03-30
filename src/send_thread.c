/**
 * @file send_thread.c Fichier source de send_thread
 * @author Floodus
 * @brief PThread envoyant des Hello et neighbors
 */

#include "send_thread.h"

/**
 * @brief
 * Envoie à tous les voisins un TLV Hello Court
 * @param list liste des voisins potentiels
 * @param nb le nombre à qui envoyer
 */
static short send_hello_short(node_t *list, int nb) {
  int rc = 0, count = 0;
  node_t *current = list;
  while(current != NULL && count < nb) {
    ip_port_t addr = {0};
    memmove(&addr, current->value->iov_base, sizeof(ip_port_t));

    struct iovec *tlv_hello = hello_short(g_myid); 
    if (tlv_hello == NULL) {
      debug(D_SEND_THREAD, 1, "send_hello_short", "tlv_hello = NULL");
      return 0;
    }
    debug_hex(D_SEND_THREAD, 0, "send_hello short", tlv_hello->iov_base, tlv_hello->iov_len);

    rc = add_tlv(addr, tlv_hello);
    if(rc < 0) {
      debug_int(D_SEND_THREAD, 1, "send_hello_short -> rc", rc);
      return rc;
    }
    freeiovec(tlv_hello);
    count++;
    current = current->next;
  }
  debug_int(D_SEND_THREAD, 0, "send_hello_short -> count", count);
  return count;
}

/**
 * @brief
 * Envoie à tous les voisins un TLV Hello Long
 * @param list liste des voisins courants
 */
static short send_hello_long(node_t *list) {
  int rc = 0, c = 0;
  node_t *current = list;
  while(current != NULL) {
    ip_port_t addr = {0};
    neighbor_t intel = {0};
    memmove(&intel, current->value->iov_base, sizeof(neighbor_t));
    memmove(&addr, current->key->iov_base, sizeof(ip_port_t));

    struct iovec *tlv_hello = hello_long(g_myid, intel.id);
    if (tlv_hello == NULL) {
      debug(D_SEND_THREAD, 1, "send_hello_short", "tlv_hello = NULL");
      return c;
    }
    debug_hex(D_SEND_THREAD, 0, "send_hello long", tlv_hello->iov_base, tlv_hello->iov_len);
    rc = add_tlv(addr, tlv_hello);
    if(rc < 0) {
      debug_int(D_SEND_THREAD, 1, "send_hello_long -> rc", rc);
      return rc;
    }
    freeiovec(tlv_hello);
    c++;
    current = current->next;
  }
  debug_int(D_SEND_THREAD, 0, "send_hello_long -> c", c);
  return c;
}

/**
 * @brief
 * Boucle d'itération du thread, envoie un Hello toutes les
 * 30 secondes
 */
static void *hello_sender(void *unused) {
  (void) unused; // Enleve le warning unused

  int count = 0;
  while(1) {
    sleep(30);
    debug(D_SEND_THREAD,0,"pthread", "Read hashmaps and send");

    lock(&g_lock_n);
    node_t *n_list = map_to_list(g_neighbors);
    unlock(&g_lock_n);
    lock(&g_lock_e);
    node_t *e_list = map_to_list(g_environs);
    unlock(&g_lock_e);

    count = send_hello_long(n_list);
    debug_int(D_SEND_THREAD, 0, "count n", count);
    if (count < MIN) {
      count = send_hello_short(e_list, MIN-count);
      debug_int(D_SEND_THREAD, 0, "count e", count);
    }

    if (n_list != NULL)
      freedeepnode(n_list);
    if (e_list != NULL)
      freedeepnode(e_list);
    debug(D_SEND_THREAD, 0, "pthread", "Sending Done");
  }
  pthread_exit(NULL);
}

/**
 * @brief
 * Declenche un nouveau thread d'envoi de Hello
 */
short init_sender() {
  pthread_t th;
  if(pthread_create(&th, NULL, hello_sender, NULL)) {
    fprintf(stderr, "Can't initialise thread sender\n");
    return 0;
  }
  return 1;
}
 


