#include "data.h"

/**
 * @brief Hashmap contenant les messages de taille conséquente
 * 
 * clé : sender_id - nonce correspondant à l'identité du gros message
 * valeur : big_data_t correspondant au gros message
 * 
 */
hashmap_t *g_big_data_map = NULL;

/**
 * @brief Initialisation de g_big_data_map
 * 
 * @return bool_t '1' si init bien passé, '0' sinon.
 */
bool_t init_big_data(void)
{
    g_big_data_map = init_map();
    if (g_big_data_map == NULL)
    {
        debug(D_DATA, 1, "init_ancillary", "impossible d'init g_big_data_map");
        return false;
    }
    debug(D_DATA, 0, "init_ancillary", "init g_big_data_map");
    return true;
}

/**
 * @brief Free de g_big_data_map
 * 
 */
void free_big_data(void)
{
    big_data_t tmp = {0};
    node_t *tmp2 = map_to_list(g_big_data_map);
    for (node_t *base = tmp2; base != NULL; base = base->next)
    {
        memmove(&tmp, (base->value->iov_base), base->value->iov_len);
        free(tmp.content);
    }
    freedeepnode(tmp2);
    freehashmap(g_big_data_map);
    g_big_data_map = NULL;
}

/**
 * @brief On regarde tout le contenu de la hashmap.
 * Si on a terminé de remplir un big_data, on l'affiche et on le supprime.
 * Si le temps pour un big_data est dépassé, on le supprime
 * 
 */
static void clear_big_data(void)
{
    struct timespec tm = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &tm) < 0)
    {
        debug(D_DATA, 1, "clear_big_data", "can't get clockgetime");
        return;
    }
    int rc = 0;
    big_data_t tmp = {0};
    node_t *tmp2 = map_to_list(g_big_data_map);
    for (node_t *base = tmp2; base != NULL; base = base->next)
    {
        rc = 0;
        memmove(&tmp, (base->value->iov_base), base->value->iov_len);
        if (tmp.read_nb == tmp.contentlen)
        {
            rc = 1;
            if (tmp.type == 0)
            {
                print_data(tmp.content, tmp.contentlen);
            }
        }
        rc = (!rc) ? compare_time(tmp.end_tm, tm) <= 0 : rc;
        if (rc)
        {
            free(tmp.content);
            remove_map(base->key, g_big_data_map);
        }
    }
    freedeepnode(tmp2);
}

/**
 * @brief On ajoute une nouvelle valeur 'big_data_t' à la hashmap
 * 
 * @param key clé de la nouvelle valeur
 * @param type type de la nouvelle valeur
 * @param contentlen taille du contenu de la nouvelle valeur
 * @return bool_t '1' si la nouvelle valeur a été ajouté, '0' sinon.
 */
static bool_t add_new_big_data(data_t key, u_int8_t type, u_int16_t contentlen)
{
    big_data_t data = {0};
    if (clock_gettime(CLOCK_MONOTONIC, &data.end_tm) < 0)
    {
        debug(D_DATA, 1, "add_new_big_data", "can't get clockgetime");
        return false;
    }
    data.end_tm.tv_sec += FIVE_MIN_SEC /* 5 min */;
    data.contentlen = contentlen;
    data.type = type;
    data.content = malloc(contentlen);
    if (data.content == NULL)
    {
        debug(D_DATA, 1, "add_new_big_data", "can't allocate memory");
        return false;
    }
    memset(data.content, 0, contentlen);
    data_t data_ivc = {&data, sizeof(big_data_t)};
    if (!insert_map(&key, &data_ivc, g_big_data_map))
    {
        free(data.content);
        debug(D_DATA, 1, "add_new_big_data", "can't insert data");
        return false;
    }
    debug(D_DATA, 0, "add_new_big_data", "insertion data");
    return true;
}

/**
 * @brief On procède au traitement d'un tlv data de type 220
 * 
 * @param sender_id id du sender
 * @param content contenu du tlv data dans le champ data
 * @return bool_t '1' si le traitement s'est bien passé, '0' sinon.
 */
static bool_t traitment_220(u_int64_t sender_id, data_t content)
{
    u_int32_t nonce = 0;
    u_int8_t type = 0;
    u_int16_t size = 0;
    u_int16_t ind = 0;
    u_int8_t *cont = content.iov_base;
    u_int8_t contlen = content.iov_len;
    memmove(&nonce, content.iov_base, sizeof(nonce));
    type = *((u_int8_t *)content.iov_base + sizeof(nonce));
    memmove(&size, ((u_int8_t *)content.iov_base + sizeof(nonce) + sizeof(type)), sizeof(size));
    memmove(&ind, ((u_int8_t *)content.iov_base + sizeof(nonce) + sizeof(type) + sizeof(size)), sizeof(ind));
    cont += sizeof(nonce) + sizeof(type) + sizeof(size) + sizeof(ind);
    contlen -= sizeof(nonce) + sizeof(type) + sizeof(size) + sizeof(ind);

    u_int8_t key_content[sizeof(sender_id) + sizeof(nonce)] = {0};
    memmove(key_content, &sender_id, sizeof(sender_id));
    memmove(key_content + sizeof(sender_id), &nonce, sizeof(nonce));
    big_data_t tmp = {0};
    data_t tmp_ivc = {&tmp, sizeof(tmp)};
    data_t key_ivc = {key_content, sizeof(sender_id) + sizeof(nonce)};

    int rc = 0;

    if (!contains_map(&key_ivc, g_big_data_map))
    {
        rc = add_new_big_data(key_ivc, type, size);
        if (!rc)
        {
            debug(D_DATA, 1, "traitment_220", "can't insert data");
            return false;
        }
    }
    rc = get_map(&key_ivc, &tmp_ivc, g_big_data_map);
    if (!rc)
    {
        debug(D_DATA, 1, "traitment_220", "can't get_map data");
        return false;
    }
    if (tmp.type != type || tmp.contentlen != size || tmp.contentlen < contlen + ind)
    {
        debug(D_DATA, 1, "traitment_220", "message non correspondant");
        return false;
    }
    tmp.read_nb += contlen;
    memmove(tmp.content + ind, cont, contlen);
    rc = insert_map(&key_ivc, &tmp_ivc, g_big_data_map);
    if (!rc)
    {
        debug(D_DATA, 1, "traitment_220", "can't insert_map data");
        return false;
    }
    debug(D_DATA, 0, "traitment_220", "traitement du data 220");
    return true;
}

/**
 * @brief On procède au traitement d'un data qu'on n'a encore jamais rencontré 
 * (selon l'horodateur des messages)
 * 
 * @param sender_id id du sender
 * @param type type du data
 * @param content contenu du data
 * @return bool_t '1' si le traitement s'est bien passé, '0' sinon.
 */
bool_t traitment_data(u_int64_t sender_id, uint8_t type, data_t content)
{
    int rc = 1;
    if (type == 0)
    {
        // action à faire quand on doit afficher une data à l'utilisateur
        print_data(content.iov_base, content.iov_len);
    }
    if (type == 220)
    {
        if (content.iov_len < 10)
        {
            debug(D_DATA, 1, "traitment_data", "taille du content trop petite");
            return false;
        }
        rc = traitment_220(sender_id, content);
        if (!rc)
            debug(D_DATA, 1, "traitment_data", "traitement du type 220 non effectué");
        else
            debug(D_DATA, 0, "traitment_data", "traitement du type 220 effectué");
    }
    clear_big_data();
    return rc;
}