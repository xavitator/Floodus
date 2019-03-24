#include "voisin.h"

/**
 * @brief Id de l'utilisateur
 * Son type est u_int64_t
 * Il sera envoyé et reçu sans se soucier du endianess
 * 
 */
u_int64_t myid = 0;

/**
 * @brief Voisins actuels
 * Hashmap dont :
 * - la key correspond à un ip_port_t
 * - la value correspond à un neighbor_t
 * 
 */
hashmap_t *neighbors = NULL;

/**
 * @brief Voisins possibles
 * Hashmap dont :
 * - la key correspond à un ip_port_t
 * - la value correspond à un ip_port_t qui est à priori le meme que la key
 * 
 */
hashmap_t *environs = NULL;


/**
 * @brief
 * Cadenas de sécurité pour les threads
 * Bloque l'accès à neighbors
 */
pthread_mutex_t lock_n;

/**
 * @brief
 * Cadenas de sécurité pour les threads
 * Bloque l'accès à environs
 */
pthread_mutex_t lock_e;

/**
 * @brief
 * Bloque le cadenas si il est libre
 * sinon attend
 * @param lock le cadenas à verrouiller
 */
short lock(pthread_mutex_t *lock) {
  int rc = 0;
  rc = pthread_mutex_lock(lock);
  if (rc)
    fprintf(stderr, "Lock mode error: lock error");
  return rc;
}

/**
 * @brief
 * Débloque le cadenas 
 * @param lock le cadenas à déverrouiller
 */
short unlock(pthread_mutex_t *lock) {
  int rc = 0;
  pthread_mutex_unlock(lock);
  if (rc)
    fprintf(stderr, "Starving mode : unlock error");
  return rc;
}

/**
 * @brief Instancie l'id du user
 * 
 */
void create_user()
{
    myid = rand();
    myid <<= 32;
    myid += rand();
}

/**
 * @brief Initialise les variables globales de voisins et de voisins possibles.
 * 
 * @return bool_t renvoie '0' s'il y a une erreur d'initialisation, '1' sinon.
 */
bool_t init_neighbors()
{
    create_user();
    neighbors = init_map();
    if (neighbors == NULL)
        return false;
    environs = init_map();
    if (environs == NULL)
    {
        freehashmap(neighbors);
        neighbors = NULL;
        return false;
    }
    return true;
}

/**
 * @brief On libère l'espace mémoire utilisé pour la gestion des voisins
 * 
 */
void free_neighbors()
{
  lock(&lock_n);
  freehashmap(neighbors);
  unlock(&lock_n);
  lock(&lock_e);
  freehashmap(environs);
  unlock(&lock_e);
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
bool_t apply_hello_court(ip_port_t ipport, u_int64_t id)
{
    data_t ipport_iovec = {&ipport, sizeof(ipport)};
    if (contains_map(&ipport_iovec, neighbors))
    {
        data_t *val = get_map(&ipport_iovec, neighbors);
        if (val == NULL)
            return false;
        neighbor_t nval = {0};
        memmove(&nval, val, sizeof(neighbor_t));
        if (id != nval.id)
            return false;
        clock_gettime(CLOCK_MONOTONIC, &nval.hello);
        data_t nval_iovec = {&nval, sizeof(nval)};
        insert_map(&ipport_iovec, &nval_iovec, neighbors);
        freeiovec(val);
    }
    else
    {
        insert_map(&ipport_iovec, &ipport_iovec, environs);
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
    if (id_dest != myid)
        return false;
    data_t ipport_iovec = {&ipport, sizeof(ipport)};
    if (contains_map(&ipport_iovec, neighbors))
    {
        data_t *val = get_map(&ipport_iovec, neighbors);
        if (val == NULL)
            return false;
        neighbor_t nval = {0};
        memmove(&nval, val, sizeof(neighbor_t));
        if (id_source != nval.id)
            return false;
        clock_gettime(CLOCK_MONOTONIC, &nval.hello);
        nval.long_hello = nval.hello;
        data_t nval_iovec = {&nval, sizeof(nval)};
        insert_map(&ipport_iovec, &nval_iovec, neighbors);
        freeiovec(val);
    }
    else
    {
        struct timespec hello = {0};
        clock_gettime(CLOCK_MONOTONIC, &hello);
        neighbor_t val = {0};
        val.hello = hello;
        val.long_hello = hello;
        val.id = id_source;
        data_t val_iovec = {&val, sizeof(val)};
        insert_map(&ipport_iovec, &val_iovec, neighbors);
        remove_map(&ipport_iovec, environs);
    }
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
    // on considère que la tete de lecture est sur l'octet correspondant à 'length' du tlv.
    if (*head_read >= data->iov_len)
        return false;
    u_int8_t lenght = 0;
    memmove(&lenght, data->iov_base + (*head_read), sizeof(u_int8_t));
    *head_read = *head_read + 1;
    switch (lenght)
    {
    case sizeof(u_int64_t):
    {
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
        return apply_hello_long(ipport, id_source, id_dest);
    }

    default:
    {
        *head_read = *head_read - 1;
        return false;
        break;
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
    // on considère que la tete de lecture est sur l'octet correspondant à 'length' du tlv.
    if (*head_read >= data->iov_len)
        return false;
    u_int8_t lenght = 0;
    memmove(&lenght, data->iov_base + (*head_read), sizeof(u_int8_t));
    if (lenght == 18 /* taille tlv_neighbour */)
    {
        *head_read = *head_read + 1;
        data_t ipport = {data + *head_read, 18};
        if (contains_map(&ipport, neighbors) == false)
            insert_map(&ipport, &ipport, environs);
        return true;
    }
    else
        return false;
}
