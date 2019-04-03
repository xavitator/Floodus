#include "inondation.h"

/**
 * @brief Variable globale contenant la liste des messages à inonder
 * 
 */
message_t *floods = NULL;

/**
 * @brief Fonction retournant si le temps donné en argument a dépassé 2 secondes.
 * 
 * @param tv temps à comparer
 * @return bool_t résultat
 */
bool_t is_symetric(struct timespec tv)
{
    struct timespec tc = {0};
    int rc = 0;
    rc = clock_gettime(CLOCK_MONOTONIC, &tc);
    if (rc < 0)
    {
        debug(D_CONTROL, 1, "is_symetric -> erreur clock_gettime", strerror(errno));
        return NULL;
    }
    bool_t res = (tc.tv_sec - tv.tv_sec) < 2;
    debug_int(D_INOND, 0, "is_symetric", res);
    return res;
}

/**
 * @brief On crée un objet de type message_t
 * 
 * @param sender couple ip-port de celui qui a envoyé le tlv data reçu
 * @param id id de l'originaire du message
 * @param nonce nonce du message envoyé
 * @param type type du message
 * @param content contenu du message envoyé
 * @param contentlen taille du message envoyé
 * @return message_t* structure construite avec toutes les données correspondantes
 */
message_t *create_message(ip_port_t sender, u_int64_t id, uint32_t nonce, uint8_t type, char *content, u_int8_t contentlen)
{
    struct timespec tc = {0};
    int rc = 0;
    message_t *res = malloc(sizeof(message_t));
    if (res == NULL)
    {
        debug(D_INOND, 1, "create_message", "création res -> problème de malloc");
        return NULL;
    }
    rc = clock_gettime(CLOCK_MONOTONIC, &tc);
    if (rc < 0)
    {
        debug(D_CONTROL, 1, "create_message -> erreur clock_gettime", strerror(errno));
        free(res);
        return NULL;
    }
    char *cont_copy = malloc(contentlen);
    if (cont_copy == NULL)
    {
        debug(D_INOND, 1, "create_message", "copy du contenu -> problème de malloc");
        free(res);
        return NULL;
    }
    hashmap_t *recipient = init_map();
    if (recipient == NULL)
    {
        debug(D_INOND, 1, "create_message", "problème de création de hashmap");
        free(res);
        free(cont_copy);
        return NULL;
    }
    lock(g_neighbors);
    node_t *neighbour = map_to_list(g_neighbors);
    unlock(g_neighbors);
    node_t *tmp = neighbour;
    memset(res, 0, sizeof(message_t));
    memmove(cont_copy, content, contentlen);
    while (neighbour != NULL)
    {
        neighbor_t voisin = {0};
        memmove(&voisin, neighbour->value, sizeof(neighbor_t));
        if (is_symetric(voisin.long_hello) && voisin.id != id && memcmp(&sender, neighbour->key, sizeof(ip_port_t)) != 0)
        {
            insert_map(neighbour->key, neighbour->key, recipient);
        }
    }
    freedeepnode(tmp);
    tc.tv_sec += 1;
    res->content = cont_copy;
    res->contentlen = contentlen;
    res->count = 0;
    res->id = id;
    res->next = NULL;
    res->nonce = nonce;
    res->recipient = recipient;
    res->send_time = tc;
    res->type = type;
    debug_hex(D_INOND, 0, "create_message -> return res", res, sizeof(message_t));
    return res;
}

/**
 * @brief Comparaison de deux struct timespec.
 * 
 * @param ta premier temps
 * @param tb deuxième temps
 * @return int Renvoie un entier inférieur, égal, ou supérieur à zéro, si 'ta' est respectivement inférieure, égale ou supérieur à 'tb'.  
 */
int compare_time(struct timespec ta, struct timespec tb)
{
    if (ta.tv_sec < tb.tv_sec)
    {
        return -1;
    }
    if (ta.tv_sec > tb.tv_sec)
    {
        return 1;
    }
    if (ta.tv_nsec < tb.tv_nsec)
    {
        return -1;
    }
    if (ta.tv_nsec > tb.tv_nsec)
    {
        return 1;
    }
    return 0;
}

/**
 * @brief On regarde si on envoie déjà le message ayant pour identifiant (id, nonce).
 * On considère le message comme un acquitement, et on enlève le 'sender' de la liste des voisins à inonder.
 * 
 * @param sender voisin qui a envoyé un tlv data
 * @param id id du data
 * @param nonce nonce du data
 * @param type type du data
 * @return bool_t '1' si on contient déjà le message et on a enlevé le voisin à inonder, '0' sinon.
 */
bool_t contains_message(ip_port_t sender, u_int64_t id, uint32_t nonce, u_int8_t type)
{
    message_t *tmp = floods;
    while (tmp != NULL)
    {
        if (tmp->id == id && tmp->nonce == nonce && tmp->type == type)
        {
            int res = remove_map(&sender, tmp->recipient);
            debug_int(D_INOND, 0, "contains_message -> on envoie deja le message et l'emetteur est enlevé des inondés", res);
            return res;
        }
    }
    debug(D_INOND, 0, "contains_message", "on n'envoie pas le message");
    return false;
}

/**
 * @brief Traitement d'un tlv data, et ajout d'une node d'inondation en cas de besoin.
 * 
 * @param sender couple ip-port de celui qui a envoyé le tlv data
 * @param id sender_id du message
 * @param nonce nonce du message
 * @param type type du message
 * @param content contenu du message
 * @param contentlen taille du contenu
 * @return bool_t '1' si le traitement a bien été fait, '0' sinon.
 */
bool_t add_message(ip_port_t sender, u_int64_t id, uint32_t nonce, uint8_t type, char *content, u_int8_t contentlen)
{
    if (contains_message(sender, id, nonce, type))
    {
        debug(D_INOND, 0, "add_message", "message en cours d'envoi");
        return true;
    }
    message_t *msg = create_message(sender, id, nonce, type, content, contentlen);
    if (msg == NULL)
    {
        debug(D_INOND, 1, "add_message", "problème de création d'un message_t");
        return false;
    }
    if (floods == NULL)
    {
        floods = msg;
        debug(D_INOND, 0, "add_message", "création première node");
        return true;
    }
    message_t *child = floods;
    message_t *father = child;
    while (child != NULL && compare_time(child->send_time, msg->send_time) < 0)
    {
        father = child;
        child = child->next;
    }
    father->next = msg;
    msg->next = child;
    return true;
}

/**
 * @brief On récupère le temps qu'il reste avant le prochain message à inonder
 * 
 * @param tm struct timespec à remplir
 * @return bool_t '1' si 'tm' a été rempli, '0' sinon.
 */
bool_t get_nexttime(struct timespec *tm)
{
    tm->tv_sec = (floods == NULL) ? 30 : floods->send_time.tv_sec;
    tm->tv_nsec = (floods == NULL) ? 0 : floods->send_time.tv_nsec;
    return true;
}
