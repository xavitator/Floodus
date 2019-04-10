/**
 * @file send_thread.c Fichier source de send_thread
 * @author Floodus
 * @brief PThread envoyant des Hello et neighbors
 */

#include "send_thread.h"

/**
 * @brief
 * Donne le temps restant nécessaire au sleep 
 * 
 * @param time le temps original
 * @param wake_up le temps auquel le thread se réveille
 * @return le temps à sleep
 */
static uint32_t get_remain_time(uint32_t TIME, struct timespec tm)
{
  struct timespec current_time = {0};
  if (clock_gettime(CLOCK_MONOTONIC, &current_time) < 0)
  {
    debug(D_SEND_THREAD, 1, "get_remain_time", "can't get clockgetime");
    return 0;
  }
  uint32_t res = TIME - (current_time.tv_sec - tm.tv_sec);
  return (res > TIME) ? 0 : res;
}

/**
 * @brief
 * Envoie les voisins à un pair
 * 
 * @param addr l'adresse de l'envoyeur
 * @param n_list la liste des voisins à envoyer
 * @return 1 si l'envoi a marché 
 */
static bool_t send_neighbour(ip_port_t *dest, node_t *n_list)
{
  node_t *node = n_list;
  ip_port_t ipport = {0};
  neighbour_t content = {0};
  memmove(&content, node->value->iov_base, sizeof(neighbour_t));
  int rc = 0;
  bool_t error = 0;
  while (node != NULL)
  {
    if (is_more_than_two(content.hello))
    {
      if (update_neighbours(node, "time out hello") == false)
        debug(D_SEND_THREAD, 1, "send_neighbour", "error with go_away");
      node = node->next;
      error = true;
      continue;
    }

    memmove(&ipport, node->key->iov_base, sizeof(ip_port_t));
    if (memcmp(dest->ipv6, ipport.ipv6, 16) != 0 ||
        dest->port != ipport.port)
    {
      data_t *tlv_neighbour = neighbour(ipport.ipv6, ipport.port);
      if (tlv_neighbour == NULL)
      {
        debug(D_SEND_THREAD, 1, "send_neighbour", "creation de tlv_neighbour impossible");
        node = node->next;
        error = true;
        freeiovec(tlv_neighbour);
        continue;
      }
      debug_hex(D_SEND_THREAD, 0, "send_neighbour -> tlv", tlv_neighbour->iov_base, tlv_neighbour->iov_len);
      rc = add_tlv(*dest, tlv_neighbour);
      if (rc == false)
      {
        debug_int(D_SEND_THREAD, 1, "send_neighbour -> rc", rc);
        node = node->next;
        error = true;
        freeiovec(tlv_neighbour);
        continue;
      }
      freeiovec(tlv_neighbour);
    }
    node = node->next;
  }
  return (error) ? false : true;
}

/**
 * @brief
 * Envoie les voisins
 * 
 * @param list liste des voisins courants
 * @return 1 si tous les voisins ont été envoyés
 */
static bool_t send_neighbours(node_t *list)
{
  int rc = 1;
  bool_t error = false;
  node_t *node = list;
  while (node != NULL)
  {
    ip_port_t ipport = {0};
    neighbour_t content = {0};
    memmove(&content, node->value->iov_base, sizeof(neighbour_t));
    memmove(&ipport, node->key->iov_base, sizeof(ip_port_t));
    rc = send_neighbour(&ipport, list);
    error = (!rc) ? true : error;
    node = node->next;
  }
  debug(D_SEND_THREAD, 0, "send_neighbours", "->neighbours");
  return (error) ? false : true;
}

/**
 * @brief
 * Boucle d'itération du thread, envoie un voisin 
 * suivant un temps aléatoire
 */
static void *neighbour_sender(void *unused)
{
  (void)unused; // Enleve le warning unused
  struct timespec tm = {0};
  if (clock_gettime(CLOCK_MONOTONIC, &tm) < 0)
  {
    debug(D_SEND_THREAD, 1, "neighbour_sender", "can't get clockgetime");
    pthread_exit(NULL);
  }
  while (1)
  {
    uint32_t remains = get_remain_time(SLEEP_NEIGHBOURS, tm);
    sleep(remains);
    if (clock_gettime(CLOCK_MONOTONIC, &tm))
    {
      debug(D_SEND_THREAD, 1, "neighbour_sender", "can't get clockgetime");
      pthread_exit(NULL);
    }
    debug(D_SEND_THREAD, 0, "pthread neighbour", "Read hashmaps and send");
    debug_int(D_SEND_THREAD, 0, "pthread neighbour", remains);

    lock(&g_lock_n);
    node_t *n_list = map_to_list(g_neighbours);
    unlock(&g_lock_n);

    send_neighbours(n_list);

    if (n_list != NULL)
      freedeepnode(n_list);
    debug(D_SEND_THREAD, 0, "pthread neighbour", "Sending neighbour done");
  }
  pthread_exit(NULL);
}

/**
 * @brief
 * Envoie à tous les voisins un TLV Hello Court
 * 
 * @param list liste des voisins potentiels
 * @param nb le nombre à qui envoyer
 * @return nombre d'hellos envoyés
 */
static int send_hello_short(node_t *list, int nb)
{
  int rc = 0, count = 0;
  node_t *node = list;
  while (node != NULL && count < nb)
  {
    ip_port_t ipport = {0};
    memmove(&ipport, node->value->iov_base, sizeof(ip_port_t));

    data_t *tlv_hello = hello_short(g_myid);
    if (tlv_hello == NULL)
    {
      debug(D_SEND_THREAD, 1, "send_hello_short", "tlv_hello = NULL");
      freeiovec(tlv_hello);
      return 0;
    }
    debug_hex(D_SEND_THREAD, 0, "send_hello short -> tlv", tlv_hello->iov_base, tlv_hello->iov_len);

    rc = add_tlv(ipport, tlv_hello);
    freeiovec(tlv_hello);
    if (rc == false)
    {
      debug_int(D_SEND_THREAD, 1, "send_hello_short -> rc", rc);
      return rc;
    }
    count++;
    node = node->next;
  }
  debug_int(D_SEND_THREAD, 0, "send_hello_short -> count", count);
  return count;
}

/**
 * @brief
 * Envoie à tous les voisins un TLV Hello Long
 * @param list liste des voisins courants
 * @return nombre d'hellos envoyés
 */
static int send_hello_long(node_t *list)
{
  int rc = 0, c = 0;
  node_t *node = list;
  while (node != NULL)
  {
    ip_port_t ipport = {0};
    neighbour_t content = {0};
    memmove(&content, node->value->iov_base, sizeof(neighbour_t));
    memmove(&ipport, node->key->iov_base, sizeof(ip_port_t));

    data_t *tlv_hello = hello_long(g_myid, content.id);
    if (tlv_hello == NULL)
    {
      debug(D_SEND_THREAD, 1, "send_hello_short", "tlv_hello = NULL");
      freeiovec(tlv_hello);
      node = node->next;
      continue;
    }
    debug_hex(D_SEND_THREAD, 0, "send_hello long", tlv_hello->iov_base, tlv_hello->iov_len);

    rc = add_tlv(ipport, tlv_hello);
    freeiovec(tlv_hello);
    if (rc == false)
    {
      debug_int(D_SEND_THREAD, 1, "send_hello_long -> rc", rc);
      node = node->next;
      continue;
    }
    c++;
    node = node->next;
  }
  debug_int(D_SEND_THREAD, 0, "send_hello_long -> c", c);
  return c;
}

/**
 * @brief
 * Boucle d'itération du thread, envoie un Hello toutes les
 * 30 secondes
 */
static void *hello_sender(void *unused)
{
  (void)unused; // Enleve le warning unused

  int count = 0;
  struct timespec tm = {0};
  if (clock_gettime(CLOCK_MONOTONIC, &tm) < 0)
  {
    debug(D_VOISIN, 1, "hello_sender", "can't get clockgetime");
    pthread_exit(NULL);
  }
  while (1)
  {
    sleep(get_remain_time(SLEEP_HELLO, tm));
    if (clock_gettime(CLOCK_MONOTONIC, &tm) < 0)
    {
      debug(D_VOISIN, 1, "hello_sender", "can't get clockgetime");
      pthread_exit(NULL);
    }
    debug(D_SEND_THREAD, 0, "pthread", "Read hashmaps and send");

    lock(&g_lock_n);
    node_t *n_list = map_to_list(g_neighbours);
    unlock(&g_lock_n);
    lock(&g_lock_e);
    node_t *e_list = map_to_list(g_environs);
    unlock(&g_lock_e);

    count = send_hello_long(n_list);
    debug_int(D_SEND_THREAD, 0, "count n", count);
    if (count < MIN)
    {
      count = send_hello_short(e_list, MIN - count);
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
 * et un d'envoi de neighbours
 *
 * @return si les threads ont été lancés
 */
short init_sender()
{
  pthread_t th1, th2;
  if (pthread_create(&th1, NULL, hello_sender, NULL))
  {
    debug(D_SEND_THREAD, 1, "pthread", "Can't initialise thread hello sender");
    return 0;
  }
  if (pthread_create(&th2, NULL, neighbour_sender, NULL))
  {
    debug(D_SEND_THREAD, 1, "pthread", "Can't initialise thread neighbour sender");
    return 0;
  }
  return 1;
}
