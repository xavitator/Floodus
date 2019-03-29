#include "writer.h"

/**
 * @brief structure contenu dans le tampon
 * - 'dest' est la destination du message
 * - 'tlvs' est un tableau de tlv à envoyer
 * - 'tlvlen' est le nombre d'éléments contenus dans 'tlvsint j = 0;
 * - 'next' est la node suivante dans le tableau
 */
typedef struct buffer_node_t
{
    ip_port_t dest;
    data_t *tlvs;
    size_t tlvlen;
    struct buffer_node_t *next;
} buffer_node_t;

/**
 * @brief Socket de réception et d'envoi des messages
 * 
 */
u_int32_t g_socket = 1;

/**
 * @brief Buffer d'écriture indexé par :
 * - la key est un ipport
 * - la value est un tableau de tlv
 */
buffer_node_t *g_write_buf = NULL;

/**
 *  @brief 
 * Cadenas bloquant l'accès concurrent au buffer
 * de messages
 */
pthread_mutex_t g_lock_buff;

/**
 * @brief On nettoie le buffer (on le free)
 * 
 */
void clear_all()
{
    buffer_node_t *tmp = NULL;
    while (g_write_buf != NULL)
    {
        tmp = g_write_buf;
        g_write_buf = g_write_buf->next;
        free(tmp->tlvs);
        free(tmp);
    }
    g_write_buf = NULL;
}

/**
 * @brief Initialisation de la socket d'envoi/écriture
 * 
 * @param socket numéro de fd de la socket
 */
void init_writer(u_int32_t socket)
{
    g_socket = socket;
}

/**
 * @brief Fonction d'envoi des tlvs à une adresse contenue dans 'dest'.
 * 
 * @param dest destination du message
 * @param database contenu du message à envoyer
 * @param len taille du contenu
 * @return bool_t '1' si le message s'est bien envoyé, '0' sinon.
 */
bool_t send_tlv(ip_port_t *ipport, data_t *database, size_t len)
{
    struct sockaddr_in6 dest = {0};
    dest.sin6_family = AF_INET6;
    dest.sin6_port = ipport->port;
    memmove(&dest.sin6_addr, ipport->ipv6, sizeof(ipport->ipv6));
    struct iovec header = {0};
    u_int8_t entete[4] = {93, 2, 0, 0};
    u_int16_t total_len = 0;
    for (size_t i = 0; i < len; i++)
    {
        total_len += database[i].iov_len;
    }
    total_len = htons(total_len);
    memmove(entete + 2, &total_len, 2);
    header.iov_base = entete;
    header.iov_len = 4;
    data_t content[len + 1];
    content[0] = header;
    for (size_t i = 0; i < len; i++)
    {
        content[i + 1] = database[i];
    }
    struct msghdr msg = {0};
    msg.msg_name = &dest;
    msg.msg_namelen = sizeof(struct sockaddr_in6);
    msg.msg_iov = content;
    msg.msg_iovlen = len + 1;
    while (1)
    {
        int rc = sendmsg(g_socket, &msg, 0);
        if (rc >= 0)
            break;
        if (errno == EINTR ||
            (errno != EWOULDBLOCK && errno != EAGAIN))
        {
            debug(D_WRITER, 1, "send_tlv -> envoi non effectué", strerror(errno));
            return false;
        }
    }
    for (size_t i = 0; i < len + 1; i++)
    {
        char buf[] = "send_tlv -> content [00000]";
        snprintf(buf, strlen(buf), "send_tlv -> content [%.4ld]", i);
        debug_hex(D_WRITER, 0, buf, content[i].iov_base, content[i].iov_len);
    }
    debug(D_WRITER, 0, "send_tlv", "demande envoyée");
    return true;
}

/**
 * @brief Est ce que 'tlv' peut être ajouté à la node 'node' du buffer pour envoyer à 'dest'?
 * Si oui, on ajoute le tlv.
 * 
 * @param dest ip-port du destinataire
 * @param tlv tlv à rajouter
 * @param node node du buffer
 * @return bool_t on renvoie '1' si l'ajout est possible et est fait, '0' sinon.
 */
bool_t can_add_tlv(ip_port_t dest, data_t *tlv, buffer_node_t *node)
{
    if (node == NULL)
    {
        debug(D_WRITER, 1, "can_add_tlv", "node = (nul)");
        return false;
    }
    if (memcmp(&(node->dest).ipv6, &dest.ipv6, sizeof(dest.ipv6)) != 0 ||
        (node->dest).port != dest.port)
    {
        debug(D_WRITER, 0, "can_add_tlv", "node ne correspond pas au destinataire");
        return false;
    }
    size_t total_len = 0;
    for (size_t i = 0; i < node->tlvlen; i++)
    {
        total_len = node->tlvs[i].iov_len;
    }
    if (total_len + tlv->iov_len > MAX_PER_TLV)
    {
        debug(D_WRITER, 0, "can_add_tlv", "taille max de tlv atteint");
        return false;
    }
    data_t *ntlv = copy_iovec(tlv);
    if (ntlv == NULL)
    {
        debug(D_WRITER, 1, "can_add_tlv", "problème de malloc -> copy de tlv");
        return false;
    }
    size_t size = sizeof(data_t) * node->tlvlen;
    data_t *content = malloc(size + sizeof(data_t));
    if (content == NULL)
    {
        freeiovec(ntlv);
        debug(D_WRITER, 1, "can_add_tlv", "cannot allow memory pour content");
        return false;
    }
    for (size_t i = 0; i < node->tlvlen; i++)
    {
        content[i] = node->tlvs[i];
    }
    content[node->tlvlen] = *ntlv;
    free(ntlv);
    free(node->tlvs);
    node->tlvs = content;
    node->tlvlen += 1;
    debug(D_WRITER, 0, "can_add_tlv", "ajout bien effectué");
    return true;
}

/**
 * @brief On ajoute le tlv 'tlv' dans le buffer d'envoi.
 * 
 * @param dest ip-port du destinataire
 * @param tlv tlv à envoyer
 * @return bool_t '1' si on a bien ajouté le tlv au buffer, '0' sinon.
 */
bool_t add_tlv(ip_port_t dest, data_t *tlv)
{
    if (tlv == NULL)
    {
        debug(D_WRITER, 1, "add_tlv", "tlv = (null)");
        return false;
    }
    lock(&g_lock_buff);
    buffer_node_t *father = g_write_buf;
    buffer_node_t *child = g_write_buf;
    while (child != NULL && can_add_tlv(dest, tlv, child) != true)
    {
        father = child;
        child = child->next;
    }
    if (child != NULL)
    {
        debug(D_WRITER, 0, "add_tlv", "ajout bien effectué");
        return true;
    }
    buffer_node_t *node = malloc(sizeof(buffer_node_t));
    if (node == NULL)
    {
        debug(D_WRITER, 1, "add_tlv", "erreur de malloc node");
        return false;
    }
    data_t *tlvs = malloc(sizeof(data_t));
    if (tlvs == NULL)
    {
        free(node);
        debug(D_WRITER, 1, "add_tlv", "erreur malloc tlvs");
        return false;
    }
    memset(node, 0, sizeof(buffer_node_t));
    memset(tlvs, 0, sizeof(data_t));
    tlvs[0] = *tlv;
    node->dest = dest;
    node->tlvs = tlvs;
    node->tlvlen = 1;
    node->next = NULL;
    if (father == NULL)
    {
        g_write_buf = node;
        debug(D_WRITER, 0, "add_tlv", "initialisation première node du buffer");
        return true;
    }
    father->next = node;
    debug(D_WRITER, 0, "add_tlv", "ajout d'une node au buffer");
    return true;
    unlock(&g_lock_buff);
}

/**
 * @brief Permet de déterminer si le buffer d'écriture est vide
 * 
 * @return bool_t '1' s'il est vide, '0' sinon.
 */
bool_t buffer_is_empty()
{
    bool_t res = (g_write_buf == NULL);
    debug_int(D_WRITER, 0, "buffer_is_empty", res);
    return res;
}

/**
 * @brief Fonction de calcul du pmtu
 * 
 * @param dest ip-port de destination
 * @return u_int32_t pmtu
 */
u_int32_t get_pmtu(ip_port_t dest)
{
    debug_hex(D_WRITER, 0, "get_pmtu -> dest", (uint8_t *)&dest, sizeof(dest));
    return 1000;
}

/**
 * @brief On envoie ce qui est en tete du buffer
 * 
 * @return bool_t '1' si tout se passe bien, '0' sinon.
 */
bool_t send_buffer_tlv()
{
    int res = 0;
    if (g_write_buf == NULL)
    {
        debug(D_WRITER, 1, "send_buffer_tlv", "le buffer est null");
        return false;
    }
    lock(&g_lock_buff);
    size_t pmtu = get_pmtu(g_write_buf->dest);
    size_t sendlen = 0;
    size_t ind = 0;
    for (; ind < g_write_buf->tlvlen; ind++)
    {
        size_t add_len = g_write_buf->tlvs[ind].iov_len;
        if (sendlen + add_len <= pmtu)
            sendlen += add_len;
        else
            break;
    }
    res = send_tlv(&g_write_buf->dest, g_write_buf->tlvs, ind);
    if (res == false)
    {
        debug(D_WRITER, 1, "send_buffer_tlv", "1er envoi non effectué");
        return false;
    }
    if (ind < g_write_buf->tlvlen)
    {
        res = send_tlv(&g_write_buf->dest, g_write_buf->tlvs + ind - 1, g_write_buf->tlvlen - ind + 1);
        if (res == false)
        {
            debug(D_WRITER, 1, "send_buffer_tlv", "2eme envoi non effectué");
            return false;
        }
    }
    for (size_t i = 0; i < g_write_buf->tlvlen; i++)
    {
        free(g_write_buf->tlvs[i].iov_base);
    }
    free(g_write_buf->tlvs);
    buffer_node_t *tmp = g_write_buf;
    g_write_buf = g_write_buf->next;
    free(tmp);
    unlock(&g_lock_buff);
    debug_int(D_WRITER, 0, "send_buffer_tlv -> envoi tlv", res);
    return res;
}