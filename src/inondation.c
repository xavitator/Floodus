#include "inondation.h"

/**
 * @brief Variable globale contenant la liste des messages à inonder
 * 
 */
message_t *g_floods = NULL;

/**
 * Cette fonction est deprecated 
 * 
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
        return false;
    }
    bool_t res = (tc.tv_sec - tv.tv_sec) < 2;
    debug_int(D_INOND, 0, "is_symetric", res);
    return res;
}

/**
 * @brief On libère la mémoire d'un message
 * 
 * @param msg message dont on libère la mémoire
 */
void freemessage(message_t *msg)
{
    free(msg->content);
    freehashmap(msg->recipient);
    free(msg);
}

/**
 * @brief On libère la mémoire d'un message et de ses suivants.
 * 
 * @param msg message dont on libère la mémoire et ses suivants
 */
void freedeepmessage(message_t *msg)
{
    message_t *tmp = msg;
    message_t *tmp2 = msg;
    while (tmp != NULL)
    {
        tmp2 = tmp->next;
        freemessage(tmp);
        tmp = tmp2;
    }
}

/**
 * @brief On libère toute la mémoire prise par l'inondation
 * 
 */
void free_inondation()
{
    freedeepmessage(g_floods);
    g_floods = NULL;
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
message_t *create_message(ip_port_t sender, u_int64_t id, uint32_t nonce, uint8_t type, u_int8_t *content, u_int8_t contentlen)
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
    u_int8_t *cont_copy = malloc(contentlen);
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
    lock(&g_lock_n);
    node_t *neighbour = map_to_list(g_neighbors);
    unlock(&g_lock_n);
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
bool_t contains_message(ip_port_t sender, u_int64_t id, uint32_t nonce)
{
    message_t *tmp = g_floods;
    while (tmp != NULL)
    {
        if (tmp->id == id && tmp->nonce == nonce)
        {
            data_t sender_iovec = {&sender, sizeof(sender)};
            int res = remove_map(&sender_iovec, tmp->recipient);
            debug_int(D_INOND, 0, "contains_message -> on envoie deja le message et l'emetteur est enlevé des inondés", res);
            return res;
        }
    }
    debug(D_INOND, 0, "contains_message", "on n'envoie pas le message");
    return false;
}

/**
 * @brief Insert le message dans la liste des messages de telle façon à garder l'odre des messages
 * 
 * @param msg message à insérer
 * @return bool_t '1' si l'insertion a eu lieu, '0' sinon.
 */
bool_t insert_message(message_t *msg)
{
    message_t *child = g_floods;
    message_t *father = child;
    while (child != NULL && compare_time(child->send_time, msg->send_time) < 0)
    {
        father = child;
        child = child->next;
    }
    if (father == child)
    {
        g_floods = msg;
        msg->next = father;
        debug(D_INOND, 0, "insert_message", "insertion en première position");
        return true;
    }
    father->next = msg;
    msg->next = child;
    debug(D_INOND, 0, "insert_message", "ajout de la node");
    return true;
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
bool_t add_message(ip_port_t sender, u_int64_t id, uint32_t nonce, uint8_t type, u_int8_t *content, u_int8_t contentlen)
{
    if (contains_message(sender, id, nonce))
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
    return insert_message(msg);
}

/**
 * @brief On récupère le temps qu'il reste avant le prochain message à inonder
 * 
 * @param tm struct timespec à remplir
 * @return bool_t '1' si 'tm' a été rempli, '0' sinon.
 */
bool_t get_nexttime(struct timespec *tm)
{

    if (g_floods == NULL)
    {
        tm->tv_sec = 30;
        tm->tv_nsec = 0;
        return true;
    }
    struct timespec tc = {0};
    int rc = clock_gettime(CLOCK_MONOTONIC, &tc);
    if (rc < 0)
    {
        debug(D_CONTROL, 1, "get_nexttime -> erreur clock_gettime", strerror(errno));
        return false;
    }
    long diff_sec = g_floods->send_time.tv_sec - tc.tv_sec;
    long diff_nsec = g_floods->send_time.tv_nsec - tc.tv_nsec;
    tm->tv_sec = (diff_sec > 0) ? diff_sec : 0;
    tm->tv_nsec = diff_nsec;
    return true;
}

/**
 * @brief Envoie de tous les tlv go_away à ceux qui n'ont pas acquitté le message
 * 
 * @param msg message qui a été inondé
 * @return bool_t '1' si l'envoi s'est bien passé, '0' sinon.
 */
bool_t flood_goaway(message_t *msg)
{
    node_t *list = map_to_list(msg->recipient);
    node_t *tmp = list;
    int rc = 0;
    char *message = "L'utilisateur n'a pas acquité le message [00000000,0000]";
    snprintf(message, strlen(message), "L'utilisateur n'a pas acquité le message [%8.lx,%4.x]", msg->id, msg->nonce);
    data_t *tlv = go_away(2, strlen(message), (u_int8_t *)message);
    while (tmp != NULL)
    {
        ip_port_t dest = {0};
        memmove(&dest, tmp->value->iov_base, sizeof(ip_port_t));
        add_tlv(dest, tlv);
        rc += 1;
        // il faudrait maintenant mettre le voisin dans la liste des voisins potentiels
        // et le supprimer des (a)symétriques
        //
        // zone à compléter :

        //
        //
        tmp = tmp->next;
    }
    freedeepnode(list);
    debug_int(D_INOND, 0, "flood_goaway -> envoi des go_away, nombre d'envoi", rc);
    return true;
}

/**
 * @brief On procède à l'inondation sur un message.
 * 
 * @param msg message à inonder
 * @return bool_t '1' si le message a été inondé.
 * '0' si on a envoyé des go_away, et donc que le message n'est pas à rajouté dans la liste des messages à inonder.
 */
bool_t flood_message(message_t *msg)
{
    if (msg->count > COUNT_INOND)
    {
        flood_goaway(msg);
        debug(D_INOND, 0, "flood_message", "envoi des goaway");
        return false;
    }
    node_t *list = map_to_list(msg->recipient);
    node_t *tmp = list;
    int rc = 0;
    data_t *tlv = data(msg->id, msg->nonce, msg->type, msg->contentlen, msg->content);
    while (tmp != NULL)
    {
        ip_port_t dest = {0};
        memmove(&dest, tmp->value->iov_base, sizeof(ip_port_t));
        add_tlv(dest, tlv);
        rc += 1;
        tmp = tmp->next;
    }
    freedeepnode(list);
    msg->count++;
    double add_time = pow((double)2, (double)msg->count);
    msg->send_time.tv_sec += (int)add_time;
    debug_int(D_INOND, 0, "flood_message -> envoi des datas, nombre d'envoi", rc);
    return true;
}

/**
 * @brief Boucle principal qui procède à l'inondation de tous les messages qui sont arrivés à maturation.
 * 
 * @return bool_t '1', si tout s'est bien passé, '0' sinon.
 */
bool_t launch_flood()
{
    message_t *msg = NULL;
    struct timespec tc = {0};
    int rc = clock_gettime(CLOCK_MONOTONIC, &tc);
    if (rc < 0)
    {
        debug(D_CONTROL, 1, "launch_flood -> erreur clock_gettime", strerror(errno));
        return false;
    }
    rc = 0;
    while (g_floods != NULL && compare_time(tc, g_floods->send_time) <= 0)
    {
        msg = g_floods;
        g_floods = g_floods->next;
        if (flood_message(msg) == false)
        {
            freemessage(msg);
        }
        else
        {
            insert_message(msg);
        }
        rc += 1;
    }
    debug_int(D_INOND, 0, "launch_flood -> nombre de messages faits", rc);
    return true;
}

/**
 * @brief Fonction qui vient faire l'action correspondante à un data pour l'inondation.
 * Si la lecture du tlv s'est bien passé, le champs 'head_read' sera modifié pour pointer vers le tlv suivant.
 * 
 * @param data Structure iovec contenant une suite de tlv.
 * @param head_read tête de lecture sur le tableau contenu dans la struct iovec.
 * @return bool_t renvoie '1' si tout s'est bien passé, '0' si on a rien fait ou s'il y a eu un problème.
 */
bool_t apply_tlv_data(ip_port_t dest, data_t *data, size_t *head_read)
{
    if (*head_read >= data->iov_len)
    {
        debug(D_INOND, 1, "apply_tlv_data", "head_read >= data->iov_len");
        return false;
    }
    uint8_t length = 0;
    memmove(&length, data->iov_base + *head_read, sizeof(u_int8_t));
    *head_read++;
    if (length < 14) //taille d'un tlv data avec au moins 1 uint8_t dans le champs message
    {
        *head_read += length;
        debug(D_INOND, 1, "apply_tlv_data", "taille trop petite");
        return false;
    }
    u_int64_t sender_id = 0;
    memmove(&sender_id, data->iov_base + *head_read, sizeof(u_int64_t));
    *head_read += sizeof(u_int64_t);
    length -= sizeof(u_int64_t);
    u_int32_t nonce = 0;
    memmove(&nonce, data->iov_base + *head_read, sizeof(u_int32_t));
    *head_read += sizeof(u_int32_t);
    length -= sizeof(u_int32_t);
    u_int8_t type = 0;
    memmove(&type, data->iov_base + *head_read, sizeof(u_int8_t));
    *head_read += sizeof(u_int8_t);
    length -= sizeof(u_int8_t);
    if (type == 0)
    {
        // action à faire quand on doit afficher une data à l'utilisateur
        print_data(data->iov_base + *head_read, length);
    }
    int rc = add_message(dest, sender_id, nonce, type, data->iov_base + *head_read, length);
    if (rc == false)
    {
        *head_read += length;
        debug(D_INOND, 1, "apply_tlv_data", "problème d'ajout du message");
        return false;
    }
    *head_read += length;
    data_t *ack_iovec = ack(sender_id, nonce);
    if (ack_iovec == NULL)
    {
        debug(D_INOND, 1, "apply_tlv_data", "problème d'envoi de l'acquitement");
        return false;
    }
    rc = add_tlv(dest, ack_iovec);
    if (rc == false)
    {
        debug(D_INOND, 1, "apply_tlv_data", "problème d'ajout du tlv ack");
        return false;
    }
    debug(D_INOND, 0, "apply_tlv_data", "traitement du tlv data effectué");
    return true;
}

/**
 * @brief Fonction qui vient faire l'action correspondante à un ack pour l'inondation.
 * Si la lecture du tlv s'est bien passé, le champs 'head_read' sera modifié pour pointer vers le tlv suivant.
 * 
 * @param data Structure iovec contenant une suite de tlv.
 * @param head_read tête de lecture sur le tableau contenu dans la struct iovec.
 * @return bool_t renvoie '1' si tout s'est bien passé, '0' si on a rien fait ou s'il y a eu un problème.
 */
bool_t apply_tlv_ack(ip_port_t dest, data_t *data, size_t *head_read)
{
    if (*head_read >= data->iov_len)
    {
        debug(D_INOND, 1, "apply_tlv_ack", "head_read >= data->iov_len");
        return false;
    }
    uint8_t length = 0;
    memmove(&length, data->iov_base + *head_read, sizeof(u_int8_t));
    *head_read++;
    if (length != 12) // taille du tlv ack
    {
        *head_read += length;
        debug(D_INOND, 1, "apply_tlv_ack", "taille trop petite");
        return false;
    }
    u_int64_t sender_id = 0;
    memmove(&sender_id, data->iov_base + *head_read, sizeof(u_int64_t));
    *head_read += sizeof(u_int64_t);
    u_int32_t nonce = 0;
    memmove(&nonce, data->iov_base + *head_read, sizeof(u_int32_t));
    *head_read += sizeof(u_int32_t);
    if (contains_message(dest, sender_id, nonce) == false)
    {
        debug(D_INOND, 0, "apply_tlv_ack", "message non en mémoire");
    }
    debug(D_INOND, 0, "apply_tlv_ack", "mise à jour du message");
    return true;
}