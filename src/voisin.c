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
 * @brief Voisins actuels
 * Hashmap dont :
 * - la key correspond à un ip_port_t
 * - la value correspond à un neighbor_t
 * 
 */
hashmap_t *g_neighbors = NULL;

/**
 * @brief Voisins possibles
 * Hashmap dont :
 * - la key correspond à un ip_port_t
 * - la value correspond à un ip_port_t qui est à priori le meme que la key
 * 
 */
hashmap_t *g_environs = NULL;

/**
 * @brief
 * Cadenas de sécurité pour les threads
 * Bloque l'accès à neighbors
 */
pthread_mutex_t g_lock_n = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief
 * Cadenas de sécurité pour les threads
 * Bloque l'accès à environs
 */
pthread_mutex_t g_lock_e = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief
 * Bloque le cadenas si il est libre
 * sinon attend
 * @param lock le cadenas à verrouiller
 * @return rc
 */
short lock(pthread_mutex_t *lock)
{
    int rc = 0;
    rc = pthread_mutex_lock(lock);
    if (rc)
    {
        debug_int(D_VOISIN, 1, "lock -> rc", rc);
        return rc;
    }
    return rc;
}

/**
 * @brief
 * Débloque le cadenas 
 * @param lock le cadenas à déverrouiller
 * @return rc 
 */
short unlock(pthread_mutex_t *lock)
{
    int rc = 0;
    rc = pthread_mutex_unlock(lock);
    if (rc)
    {
        debug_int(D_VOISIN, 1, "unlock -> rc", rc);
        return rc;
    }
    return rc;
}

/**
 * @brief Instancie l'id du user
 * 
 */
void create_user()
{
    g_myid = rand();
    g_myid <<= 32;
    g_myid += rand();
    debug_hex(D_VOISIN, 0, "create_user -> id", &g_myid, sizeof(g_myid));
}


/**
 * @brief Initialise les variables globales de voisins et de voisins possibles.
 * 
 * @return bool_t renvoie '0' s'il y a une erreur d'initialisation, '1' sinon.
 */
bool_t init_neighbors()
{
    create_user();
    g_neighbors = init_map();
    if (g_neighbors == NULL)
    {
        debug(D_VOISIN, 1, "init_neighbors", "neighbors = NULL");
        return false;
    }
    g_environs = init_map();
    if (g_environs == NULL)
    {
        debug(D_VOISIN, 1, "init_neighbors", "environs = NULL");
        freehashmap(g_neighbors);
        g_neighbors = NULL;
        return false;
    }
    debug(D_VOISIN, 0, "init_neighbors", "init all environments");
    return true;
}


/**
 * @brief
 * Deplace les données de la hashmap g_neighbors
 * vers g_environ
 * 
 * @param addr l'adresse à déplacer dans la hashmap
 * @return true si le déplacement a fonctionné
 */
static bool_t from_neighbours_to_env(ip_port_t *addr) {
  data_t addr_iov = {&addr, sizeof(ip_port_t)};
  lock(&g_lock_e);
  if(insert_map(&addr_iov, &addr_iov, g_environs) == false)
    return false;
  unlock(&g_lock_e);
  lock(&g_lock_n);
  if(remove_map(&addr_iov, g_neighbors) == false)
    return false;
  unlock(&g_lock_n);
  return true;
}


/**
 * Ajoute un GoAway à la liste des envois
 * 
 * @param addr l'adresse à laquelle envoyer le goaway
 * @param msg le message à envoyer
 * @return true si le tlv go_away a été envoyé
 */
static bool_t inform_neighbor(ip_port_t *addr, char *msg) {
    int rc;
    data_t *tlv_go_away = go_away(2, strlen(msg), (uint8_t*)msg);
    if(tlv_go_away == NULL) {
      debug(D_VOISIN, 1, "inform_neighbor", "tlv_go_away = NULL");
      return false;
    }
    debug_hex(D_VOISIN, 0, "inform_neighbor -> tlv", tlv_go_away->iov_base, tlv_go_away->iov_len);
    rc = add_tlv(*addr, tlv_go_away);
    if(rc == false) {
      debug_int(D_VOISIN, 1, "send_neighbour -> rc", rc);
    }
      return rc;
}


/**
 * @brief
 * Fait passer un voisin de voisin à
 * voisin potentiel et envoie un tlv 
 * go away.
 * 
 * @param current le node à tester 
 * @param msg le message à envoyer
 * @return true en cas de succès
 */
bool_t update_neighbours(node_t *current, char *msg) {
  ip_port_t addr = {0};
  memmove(&addr, current->key->iov_base, sizeof(ip_port_t));
  if(from_neighbours_to_env(&addr) == false)
    return false;
  if(inform_neighbor(&addr, msg) == false)
    return false;
  return true;
}


/**
 * @brief On libère l'espace mémoire utilisé pour la gestion des voisins
 * 
 */
void free_neighbors()
{
    lock(&g_lock_n);
    freehashmap(g_neighbors);
    unlock(&g_lock_n);
    lock(&g_lock_e);
    freehashmap(g_environs);
    unlock(&g_lock_e);
    debug(D_VOISIN, 0, "free_neighbors", "free env");
}

/**
 * @brief Fonction qui fait les actions correspondantes à la réception d'un hello court.
 * Si 'ipport' correspond à un élément dans les voisins, on met à jour le temps de réception du dernier hello.
 * Sinon, on ajoute l'utilisateur dans la table des voisins possibles.
 * 
 * @param ipport structure ip_port_t contenant les infos de celui qui envoie le hello court.
 * @param id id contenu dans le message hello court
 * @return bool_t '1' si tout s'est bien passé, '0' sinon.
 */
bool_t apply_hello_court(ip_port_t ipport, u_int64_t id) {
    debug_hex(D_VOISIN, 0, "apply_hello_court -> args ipport", &ipport, sizeof(ip_port_t));
    debug_hex(D_VOISIN, 0, "apply_hello_court -> args id", &id, sizeof(u_int64_t));

    data_t ipport_iovec = {&ipport, sizeof(ipport)};
    neighbor_t nval = {0};
    
    lock(&g_lock_n);
    lock(&g_lock_e);
    if (contains_map(&ipport_iovec, g_neighbors)) {
        debug(D_VOISIN, 0, "apply_hello_court", "update neighbor");
        data_t *val = get_map(&ipport_iovec, g_neighbors);
        if (val == NULL) {
            debug(D_VOISIN, 1, "apply_hello_court", "val = NULL");
            unlock(&g_lock_n);
            unlock(&g_lock_e);
            return false;
        }
        memmove(&nval, val->iov_base, sizeof(neighbor_t));
        if (id != nval.id) {
            freeiovec(val);
            debug(D_VOISIN, 1, "apply_hello_court", "id != nval.id");
            unlock(&g_lock_n);
            unlock(&g_lock_e);
            return false;
        }
        freeiovec(val);
    } else  {
        debug(D_VOISIN, 0, "apply_hello_court", "insert new neighbor");
        nval.id = id;
        remove_map(&ipport_iovec, g_environs);
    }

    clock_gettime(CLOCK_MONOTONIC, &nval.hello);
    data_t nval_iovec = {&nval, sizeof(nval)};
    insert_map(&ipport_iovec, &nval_iovec, g_neighbors);
    
    unlock(&g_lock_n);
    unlock(&g_lock_e);

    struct iovec *tlv_hello = hello_long(g_myid, nval.id);
    
    if (tlv_hello == NULL) {
      debug(D_VOISIN, 1, "apply_hello_court", "tlv_hello = NULL");
      return false;
    }
    
    debug_hex(D_VOISIN, 0, "apply_hello_court -> tlv_hello", tlv_hello->iov_base, tlv_hello->iov_len);
    int rc = add_tlv(ipport, tlv_hello);
    freeiovec(tlv_hello);
    if(rc == false) {
      debug_int(D_VOISIN, 1, "send_hello_long -> rc", rc);
      return rc;
    }
    return true;
}

/**
 * @brief Fonction qui fait les actions correspondantes à la réception d'un hello long.
 * Si 'id_dest' n'est pas l'id de celui qui reçoit le tlv, on ne fait rien et on renvoie '0'.
 * Si 'ipport' correspond à un élément dans les voisins, on met à jour les temps de réception du dernier hello/hello long.
 * Sinon, on le spécifie comme étant un nouveau voisin symétrique, et on l'enlève de la table des voisins possibles.
 * 
 * @param ipport structure ip_port_t contenant les infos de celui qui envoie le hello court.
 * @param id_source id de celui qui envoie le tlv hello long.
 * @param id_dest id de celui à qui était destiné ce tlv
 * @return bool_t '1' si tout s'est bien passé, '0' sinon.
 */
bool_t apply_hello_long(ip_port_t ipport, u_int64_t id_source, u_int64_t id_dest)
{
    debug_hex(D_VOISIN, 0, "apply_hello_long -> args ipport", &ipport, sizeof(ip_port_t));
    debug_hex(D_VOISIN, 0, "apply_hello_long -> args id_source", &id_source, sizeof(u_int64_t));
    debug_hex(D_VOISIN, 0, "apply_hello_long -> args id_dest", &id_dest, sizeof(u_int64_t));

    if (id_dest != g_myid)
    {
        debug(D_VOISIN, 1, "apply_hello_long", "wrong id : id_dest != myid");
        return false;
    }

    data_t ipport_iovec = {&ipport, sizeof(ipport)};
    neighbor_t nval = {0};
    lock(&g_lock_n);
    lock(&g_lock_e);
    if (contains_map(&ipport_iovec, g_neighbors))
    {
        data_t *val = get_map(&ipport_iovec, g_neighbors);
        if (val == NULL)
        {
            debug(D_VOISIN, 1, "apply_hello_long", "val = NULL");
            unlock(&g_lock_n);
            unlock(&g_lock_e);
            return false;
        }
        memmove(&nval, val->iov_base, sizeof(neighbor_t));
        if (id_source != nval.id)
        {
            freeiovec(val);
            debug(D_VOISIN, 1, "apply_hello_long", "id_source != nval.id");
            unlock(&g_lock_n);
            unlock(&g_lock_e);
            return false;
        }
        freeiovec(val);
        debug(D_VOISIN, 0, "apply_hello_long", "update neighbor");
    } else {
        nval.id = id_source;
        remove_map(&ipport_iovec, g_environs);
        debug(D_VOISIN, 0, "apply_hello_long", "insert new neighbor");
    }

    clock_gettime(CLOCK_MONOTONIC, &nval.hello);
    nval.long_hello = nval.hello;
    data_t nval_iovec = {&nval, sizeof(nval)};
    insert_map(&ipport_iovec, &nval_iovec, g_neighbors);
    
    unlock(&g_lock_n);
    unlock(&g_lock_e);
    debug_hex(D_VOISIN, 0, "apply_hello_long -> nval", &nval, sizeof(neighbor_t));
    return true;
}

/**
 * @brief Fonction qui vient faire l'action correspondante à un hello court ou long au niveau des tables de voisinages.
 * Si la lecture du tlv s'est bien passé, le champs 'head_read' sera modifié pour pointer vers le tlv suivant.
 * 
 * @param ipport structure identifiant celui qui a envoyé le tlv. Se referer à la structure 'ip_port_t'. 
 * @param data Structure iovec contenant une suite de tlv.
 * @param head_read tête de lecture sur le tableau contenu dans la struct iovec.
 * @return bool_t renvoie '1' si tout s'est bien passé, '0' si on a rien fait ou s'il y a eu un problème.
 */
bool_t apply_tlv_hello(ip_port_t ipport, data_t *data, size_t *head_read)
{
    debug_hex(D_VOISIN, 0, "apply_tlv_hello -> args ipport", &ipport, sizeof(ip_port_t));
    debug_hex(D_VOISIN, 0, "apply_tlv_hello -> args data", data->iov_base, data->iov_len);
    debug_int(D_VOISIN, 0, "apply_tlv_hello -> args head_read", *head_read);
    // on considère que la tete de lecture est sur l'octet correspondant à 'length' du tlv.
    if (*head_read >= data->iov_len)
    {
        debug(D_VOISIN, 1, "apply_tlv_hello", "head > data_iov");
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
        memmove(&id, data + *head_read, sizeof(u_int64_t));
        *head_read = *head_read + sizeof(u_int64_t);
        return apply_hello_court(ipport, id);
    }

    case 2 * sizeof(u_int64_t):
    {
        u_int64_t id_source = 0;
        u_int64_t id_dest = 0;
        memmove(&id_source, data->iov_base + *head_read, sizeof(id_source));
        *head_read = *head_read + sizeof(id_source);
        memmove(&id_dest, data->iov_base + *head_read, sizeof(id_dest));
        *head_read = *head_read + sizeof(id_dest);
        debug(D_VOISIN, 0, "apply_tlv_hello", "TLV long");
        return apply_hello_long(ipport, id_source, id_dest);
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
    debug_hex(D_VOISIN, 0, "apply_tlv_neighbour -> args data", data->iov_base, data->iov_len);
    debug_int(D_VOISIN, 0, "apply_tlv_neighbour -> args head_read", *head_read);
    // on considère que la tete de lecture est sur l'octet correspondant à 'length' du tlv.
    if (*head_read >= data->iov_len)
    {
        debug(D_VOISIN, 1, "apply_tlv_neighbour", "head_read >= data->iov_");
        return false;
    }
    u_int8_t length = 0;
    memmove(&length, data->iov_base + (*head_read), sizeof(u_int8_t));
    if (length == 18 /* taille tlv_neighbour */)
    {
        debug(D_VOISIN, 0, "apply_tlv_neighbour", "right neighbor");
        *head_read = *head_read + 1;
        data_t ipport = {data->iov_base + *head_read, 18};
        lock(&g_lock_n);
        lock(&g_lock_e);
        if (contains_map(&ipport, g_neighbors) == false)
        {
            debug(D_VOISIN, 0, "apply_tlv_neighbour", "insert new neighbour");
            insert_map(&ipport, &ipport, g_environs);
        }
        unlock(&g_lock_e);
        unlock(&g_lock_n);
        *head_read += 18;
        return true;
    }
    else
    {
        *head_read += length; // En cas d'erreur on ignore la donnée
        debug(D_VOISIN, 1, "apply_tlv_neighbour", "wrong neighbour size");
        return false;
    }
}

/*
 * Renvoie true si c'est un voisin symétrique
 */
bool_t is_symetric(ip_port_t ip) {
  struct iovec ip_iovec = {&ip, sizeof(ip_port_t)};
  data_t *value = get_map(&ip_iovec, g_neighbors);
  if(value != NULL) {
    neighbor_t tmp_neighbor = {0};
    memmove(&tmp_neighbor, value->iov_base, value->iov_len);
    if(is_more_than_two(tmp_neighbor.long_hello) == false) {
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
 * @param node_tv temps du dernier hello long
 * @return true si le temps est supérieur à 2min
 */
bool_t is_more_than_two(struct timespec node_tv) {
  struct timespec tv_current = {0};
  if(clock_gettime(CLOCK_MONOTONIC, &tv_current)) {
    debug(D_VOISIN, 1, "is_asymetric", "can't get clockgetime");
    return false;
  }
  if(tv_current.tv_sec - node_tv.tv_sec > 120)
    return true;
  return false;
}


