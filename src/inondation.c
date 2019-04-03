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
 * @param content contenu du message envoyé
 * @param contentlen taille du message envoyé
 * @return message_t* structure construite avec toutes les données correspondantes
 */
message_t *create_message(ip_port_t sender, u_int64_t id, uint32_t nonce, char *content, u_int8_t contentlen)
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
    debug_hex(D_INOND, 0, "create_message -> return res", res, sizeof(message_t));
    return res;
}