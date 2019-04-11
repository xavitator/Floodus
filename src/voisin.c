/**
 * @file voisin.c
 * @author Floodus
 * @brief Module s'occupant de tout ce qui est voisinage : voisins possibles, voisins asymétriques, voisins symétriques.
 * Il contient les fonctions d'interprétation des hello courts/long et des neighbours.
 * 
 */

#include "voisin.h"

/**
 * @brief Id de l'utilisateur
 * Son type est u_int64_t
 * Il sera envoyé et reçu sans se soucier du endianess
 * 
 */
u_int64_t g_myid = 0;

/**
 * @brief
 * Variable encapsulant une hashmap de voisins
 */
pthread_var_t g_neighbours = {0};


/**
 * @brief 
 * Variable encaspulant une hashmap de voisins potentiels
 */
pthread_var_t g_environs = {0};


/**
 * @brief Instancie l'id du user
 * 
 */
void create_user()
{
  srand(time(NULL));
  g_myid = rand();
  g_myid <<= 32;
  g_myid += rand();
  debug_hex(D_VOISIN, 0, "create_user -> id", &g_myid, sizeof(g_myid));
}

/**
 * Initialise les structures d'encapsulation
 * des hasmaps
 */
static void init_lockers() {
  //pthread_mutex_t lock_test = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_init(&g_neighbours.locker, NULL);
  g_neighbours.content = NULL;
  pthread_mutex_init(&g_environs.locker, NULL);
  g_environs.content = NULL;
}


/**
 * @brief Initialise les variables globales de voisins et de voisins possibles.
 * 
 * @return bool_t renvoie '0' s'il y a une erreur d'initialisation, '1' sinon.
 */
bool_t init_neighbours()
{
  init_lockers();
  create_user();
  lock(&g_neighbours);
  g_neighbours.content = init_map();
  if (g_neighbours.content == NULL)
  {
    debug(D_VOISIN, 1, "init_neighbors", "neighbors = NULL");
    return false;
  }
  unlock(&g_neighbours);
  lock(&g_environs);
  g_environs.content = init_map();
  if (g_environs.content == NULL)
  {
    lock(&g_neighbours);
    debug(D_VOISIN, 1, "init_neighbors", "environs = NULL");
    freehashmap(g_neighbours.content);
    unlock(&g_neighbours);
    return false;
  }
  unlock(&g_environs);
  debug(D_VOISIN, 0, "init_neighbors", "init all environments");
  return true;
}

/**
 * @brief
 * Renvoie la hasmap d'une variable
 *
 * @param h_var la structure contenant la hashmap
 */
hashmap_t * get_hashmap_from(pthread_var_t *h_var) {
  return (hashmap_t *) h_var->content;
}

/**
 * @brief
 * Deplace les données de la hashmap g_neighbors
 * vers g_environ
 * 
 * @param ipport l'adresse à déplacer dans la hashmap
 * @return true si le déplacement a fonctionné
 */
static bool_t from_neighbours_to_env(ip_port_t ipport)
{
  data_t ipport_ivc = {&ipport, sizeof(ip_port_t)};
  lock(&g_environs);
  if (insert_map(&ipport_ivc, &ipport_ivc, get_hashmap_from(&g_environs)) == false)
  {
    unlock(&g_environs);
    debug(D_VOISIN, 1, "from_neighbour_to_env", "insertion dans g_environs non effectuée");
    return false;
  }
  unlock(&g_environs);
  lock(&g_neighbours);
  if (remove_map(&ipport_ivc, get_hashmap_from(&g_neighbours)) == false)
  {
    unlock(&g_neighbours);
    debug(D_VOISIN, 1, "from_neighbour_to_env", "suppression de g_neighbours non effectuée");
    return false;
  }
  unlock(&g_neighbours);
  debug(D_VOISIN, 0, "from_neighbour_to_env", "déplacement effectué");
  return true;
}

/**
 * Ajoute un GoAway à la liste des envois
 * 
 * @param dest l'adresse à laquelle envoyer le goaway
 * @param msg le message à envoyer
 * @return true si le tlv go_away a été envoyé
 */
static bool_t inform_neighbor(ip_port_t dest, char *msg)
{
  int rc;
  data_t tlv_go_away;
  if(!go_away(&tlv_go_away, 2, (uint8_t *)msg, strlen(msg)))
  {
    debug(D_VOISIN, 1, "inform_neighbor", "tlv_go_away = NULL");
    return false;
  }
  rc = add_tlv(dest, &tlv_go_away);
  if (rc == false)
  {
    debug_int(D_VOISIN, 1, "inform_neighbor -> rc", rc);
  }
  return rc;
}

/**
 * @brief
 * Fait passer un voisin de voisin à
 * voisin potentiel et envoie un tlv 
 * go away.
 * 
 * @param node le node à tester 
 * @param msg le message à envoyer
 * @return true en cas de succès
 */
bool_t update_neighbours(node_t *node, char *msg)
{
  ip_port_t ipport = {0};
  memmove(&ipport, node->key->iov_base, sizeof(ip_port_t));
  if (from_neighbours_to_env(ipport) == false)
  {
    debug(D_VOISIN, 1, "update_neighbours", "voisin non déplacé");
    return false;
  }
  if (inform_neighbor(ipport, msg) == false)
  {
    debug(D_VOISIN, 1, "update_neighbours", "voisin non informé");
    return false;
  }
  debug(D_VOISIN, 0, "update_neighbours", "udpate effectuée");
  return true;
}

/**
 * @brief On libère l'espace mémoire utilisé pour la gestion des voisins
 * 
 */
void free_neighbors()
{
  lock(&g_neighbours);
  freehashmap(g_neighbours.content);
  unlock(&g_neighbours);
  lock(&g_environs);
  freehashmap(g_environs.content);
  unlock(&g_environs);
  debug(D_VOISIN, 0, "free_neighbors", "free env");
}

/**
 * @brief Fonction qui fait les actions correspondantes à la réception d'un hello court.
 * Si 'src' correspond à un élément dans les voisins, on met à jour le temps de réception du dernier hello.
 * Sinon, on ajoute l'utilisateur dans la table des voisins possibles.
 * 
 * @param src structure ip_port_t contenant les infos de celui qui envoie le hello court.
 * @param id id contenu dans le message hello court
 * @return bool_t '1' si tout s'est bien passé, '0' sinon.
 */
bool_t apply_hello_court(ip_port_t src, u_int64_t id)
{
  //debug_hex(D_VOISIN, 0, "apply_hello_court -> args src", &src, sizeof(ip_port_t));
  //debug_hex(D_VOISIN, 0, "apply_hello_court -> args id", &id, sizeof(u_int64_t));

  int rc = 0;
  data_t src_ivc = {&src, sizeof(ip_port_t)};
  neighbour_t nval = {0};

  lock(&g_neighbours);
  lock(&g_environs);
  if (contains_map(&src_ivc, get_hashmap_from(&g_neighbours)))
  {
    debug(D_VOISIN, 0, "apply_hello_court", "update neighbor");
    data_t *val = get_map(&src_ivc, get_hashmap_from(&g_neighbours));
    if (val == NULL)
    {
      debug(D_VOISIN, 1, "apply_hello_court", "val = NULL");
      unlock(&g_neighbours);
      unlock(&g_environs);
      freeiovec(val);
      return false;
    }
    memmove(&nval, val->iov_base, sizeof(neighbour_t));
    if (id != nval.id)
    {
      freeiovec(val);
      debug(D_VOISIN, 1, "apply_hello_court", "id != nval.id");
      unlock(&g_neighbours);
      unlock(&g_environs);
      return false;
    }
    freeiovec(val);
  }
  else
  {
    debug(D_VOISIN, 0, "apply_hello_court", "insert new neighbour");
    nval.id = id;
    remove_map(&src_ivc, get_hashmap_from(&g_environs));
  }

  rc = clock_gettime(CLOCK_MONOTONIC, &nval.hello);
  if (rc < 0)
  {
    debug(D_VOISIN, 1, "apply_hello_court", "clock_gettime wrong");
    return false;
  }
  data_t nval_ivc = {&nval, sizeof(nval)};
  insert_map(&src_ivc, &nval_ivc, get_hashmap_from(&g_neighbours));

  unlock(&g_neighbours);
  unlock(&g_environs);

  data_t tlv_hello;
  if(!hello_long(&tlv_hello, g_myid, nval.id)) 
  {
    debug(D_VOISIN, 1, "apply_hello_court", "tlv_hello = NULL");
    return false;
  }

  rc = add_tlv(src, &tlv_hello);
  if (rc == false)
  {
    debug_int(D_VOISIN, 1, "apply_hello_court -> rc", rc);
    return rc;
  }
  return true;
}

/**
 * @brief Fonction qui fait les actions correspondantes à la réception d'un hello long.
 * Si 'id_dest' n'est pas l'id de celui qui reçoit le tlv, on ne fait rien et on renvoie '0'.
 * Si 'src' correspond à un élément dans les voisins, on met à jour les temps de réception du dernier hello/hello long.
 * Sinon, on le spécifie comme étant un nouveau voisin symétrique, et on l'enlève de la table des voisins possibles.
 * 
 * @param src structure ip_port_t contenant les infos de celui qui envoie le hello court.
 * @param id_src id de celui qui envoie le tlv hello long.
 * @param id_dest id de celui à qui était destiné ce tlv
 * @return bool_t '1' si tout s'est bien passé, '0' sinon.
 */
bool_t apply_hello_long(ip_port_t src, u_int64_t id_src, u_int64_t id_dest)
{
  //debug_hex(D_VOISIN, 0, "apply_hello_long -> args src", &src, sizeof(ip_port_t));
  //debug_hex(D_VOISIN, 0, "apply_hello_long -> args id_src", &id_src, sizeof(u_int64_t));
  //debug_hex(D_VOISIN, 0, "apply_hello_long -> args id_dest", &id_dest, sizeof(u_int64_t));

  if (id_dest != g_myid)
  {
    debug(D_VOISIN, 1, "apply_hello_long", "wrong id : id_dest != myid");
    return false;
  }

  int rc = 0;
  data_t src_ivc = {&src, sizeof(src)};
  neighbour_t nval = {0};
  lock(&g_neighbours);
  lock(&g_environs);
  if (contains_map(&src_ivc, get_hashmap_from(&g_neighbours)))
  {
    data_t *val = get_map(&src_ivc, get_hashmap_from(&g_neighbours));
    if (val == NULL)
    {
      debug(D_VOISIN, 1, "apply_hello_long", "val = NULL");
      unlock(&g_neighbours);
      unlock(&g_environs);
      freeiovec(val);
      return false;
    }
    memmove(&nval, val->iov_base, sizeof(neighbour_t));
    if (id_src != nval.id)
    {
      freeiovec(val);
      debug(D_VOISIN, 1, "apply_hello_long", "id_src != nval.id");
      unlock(&g_neighbours);
      unlock(&g_environs);
      return false;
    }
    freeiovec(val);
  }
  else
  {
    nval.id = id_src;
    remove_map(&src_ivc, get_hashmap_from(&g_environs));
  }

  rc = clock_gettime(CLOCK_MONOTONIC, &nval.hello);
  if (rc < 0)
  {
    debug(D_VOISIN, 1, "apply_hello_long", "clock_gettime wrong");
    return false;
  }
  nval.long_hello = nval.hello;
  data_t nval_ivc = {&nval, sizeof(nval)};
  insert_map(&src_ivc, &nval_ivc, get_hashmap_from(&g_neighbours));

  unlock(&g_neighbours);
  unlock(&g_environs);
  debug(D_VOISIN, 0, "apply_hello_long", "interprétation effectuée");
  return true;
}

/**
 * @brief Fonction qui vient faire l'action correspondante à un hello court ou long au niveau des tables de voisinages.
 * Si la lecture du tlv s'est bien passé, le champs 'head_read' sera modifié pour pointer vers le tlv suivant.
 * 
 * @param src structure identifiant celui qui a envoyé le tlv. Se referer à la structure 'ip_port_t'. 
 * @param data Structure iovec contenant une suite de tlv.
 * @param head_read tête de lecture sur le tableau contenu dans la struct iovec.
 * @return bool_t renvoie '1' si tout s'est bien passé, '0' si on a rien fait ou s'il y a eu un problème.
 */
bool_t apply_tlv_hello(ip_port_t src, data_t *data, size_t *head_read)
{
  if (*head_read >= data->iov_len)
  {
    debug(D_VOISIN, 1, "apply_tlv_hello", "head >= data->iov_len");
    return false;
  }
  u_int8_t length = 0;
  memmove(&length, data->iov_base + (*head_read), sizeof(u_int8_t));
  *head_read = *head_read + 1;
  switch (length)
  {
  case sizeof(u_int64_t):
  {
    debug(D_VOISIN, 0, "apply_tlv_hello", "TLV court");
    u_int64_t id = 0;
    memmove(&id, data->iov_base + *head_read, sizeof(u_int64_t));
    *head_read = *head_read + sizeof(u_int64_t);
    return apply_hello_court(src, id);
  }

  case 2 * sizeof(u_int64_t):
  {
    debug(D_VOISIN, 0, "apply_tlv_hello", "TLV long");
    u_int64_t id_src = 0;
    u_int64_t id_dest = 0;
    memmove(&id_src, data->iov_base + *head_read, sizeof(id_src));
    *head_read = *head_read + sizeof(id_src);
    memmove(&id_dest, data->iov_base + *head_read, sizeof(id_dest));
    *head_read = *head_read + sizeof(id_dest);
    return apply_hello_long(src, id_src, id_dest);
  }

  default:
  {
    *head_read = *head_read + length;
    debug(D_VOISIN, 1, "apply_tlv_hello", "wrong TLV Hello");
    return false;
  }
  }
  return true;
}

/**
 * @brief Fonction qui vient faire l'action correspondante à un neighbour au niveau des tables de voisinages.
 * Si la lecture du tlv s'est bien passé, le champs 'head_read' sera modifié pour pointer vers le tlv suivant.
 * 
 * @param data Structure iovec contenant une suite de tlv.
 * @param head_read tête de lecture sur le tableau contenu dans la struct iovec.
 * @return bool_t renvoie '1' si tout s'est bien passé, '0' si on a rien fait ou s'il y a eu un problème.
 */
bool_t apply_tlv_neighbour(data_t *data, size_t *head_read)
{
  if (*head_read >= data->iov_len)
  {
    debug(D_VOISIN, 1, "apply_tlv_neighbour", "head_read >= data->iov_len");
    return false;
  }
  u_int8_t length = 0;
  memmove(&length, data->iov_base + (*head_read), sizeof(u_int8_t));
  *head_read = *head_read + 1;
  if (length == 18 /* taille tlv_neighbour */)
  {
    data_t ipport = {data->iov_base + *head_read, 18};
    lock(&g_neighbours);
    lock(&g_environs);
    if (!contains_map(&ipport, get_hashmap_from(&g_neighbours)))
    {
      debug(D_VOISIN, 0, "apply_tlv_neighbour", "insert new neighbour");
      insert_map(&ipport, &ipport, get_hashmap_from(&g_environs));
    }
    unlock(&g_environs);
    unlock(&g_neighbours);
    *head_read += length;
    debug(D_VOISIN, 0, "apply_tlv_neighbour", "traitement effectué");
    return true;
  }
  else
  {
    *head_read += length; // En cas d'erreur on ignore la donnée
    debug(D_VOISIN, 1, "apply_tlv_neighbour", "wrong neighbour size");
    return false;
  }
}

/**
 * @brief
 * Applique un TLV Go_away
 *
 * @param dest l'ip/port du message
 * @param data la donnée
 * @param head_read la tete de lecture
 * @return true si il a pu l'appliquer
 */
bool_t apply_tlv_goaway(ip_port_t dest, data_t *data, size_t *head_read)
{
  if (*head_read >= data->iov_len)
  {
    debug(D_VOISIN, 1, "apply_tlv_go_away", "head_read >= data->iov_len");
    return false;
  }
  u_int8_t length = 0;
  memmove(&length, data->iov_base + (*head_read), sizeof(u_int8_t));
  *head_read += 1;

  u_int8_t code = 0;
  memmove(&code, data->iov_base + (*head_read), sizeof(u_int8_t));
  *head_read += 1;
  length -= 1;

  char *msg = malloc(length + 1);
  memmove(msg, data->iov_base + *head_read, length);
  msg[length] = '\0';
  *head_read += length;
  if (code == 0 || code == 2 || code == 3)
  {
    debug(D_VOISIN, 1, "apply_tlv_goaway -> 0/2/3", msg);
    if (from_neighbours_to_env(dest) == false)
    {
      free(msg);
      return false;
    }
    free(msg);
    return true;
  }
  else if (code == 1)
  {
    data_t dest_ivc = {&dest, sizeof(ip_port_t)};
    debug(D_VOISIN, 1, "apply_tlv_goaway -> 1", msg);
    lock(&g_neighbours);
    lock(&g_environs);
    if (!remove_map(&dest_ivc, get_hashmap_from(&g_neighbours)) 
        && !remove_map(&dest_ivc, get_hashmap_from(&g_environs)))
    {
      unlock(&g_neighbours);
      unlock(&g_environs);
      free(msg);
      return false;
    }
    unlock(&g_neighbours);
    unlock(&g_environs);
    free(msg);
    return true;
  }
  else
  {
    free(msg);
    debug(D_VOISIN, 1, "apply_tlv_goaway", "wrong go_away size");
    return false;
  }
}

/**
 * @brief Renvoie l'ipport correspond à un voisin symétrique
 * 
 * @param ipport couple ip-port du voisin
 * @return bool_t Renvoie true si c'est un voisin symétrique
 */
bool_t is_symetric(ip_port_t ipport)
{
  struct iovec ipport_iovec = {&ipport, sizeof(ip_port_t)};
  data_t *value = get_map(&ipport_iovec, get_hashmap_from(&g_neighbours));
  if (value != NULL)
  {
    neighbour_t tmp_neighbour = {0};
    memmove(&tmp_neighbour, value->iov_base, value->iov_len);
    if (!is_more_than_two(tmp_neighbour.long_hello))
    {
      freeiovec(value);
      return true;
    }
  }
  freeiovec(value);
  return false;
}

/**
 * @brief
 * Retourne si un voisin est asymétrique ou non
 *
 * @param tm temps du dernier hello long
 * @return true si le temps est supérieur à 2min
 */
bool_t is_more_than_two(struct timespec tm)
{
  struct timespec tv_current = {0};
  if (clock_gettime(CLOCK_MONOTONIC, &tv_current) < 0)
  {
    debug(D_VOISIN, 1, "is_more_than_two", "can't get clockgetime");
    return false;
  }
  return (tv_current.tv_sec - tm.tv_sec > 120);
}
