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
    struct sockaddr_in6 dest;
    data_t **tlvs;
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
    int rc = sendmsg(g_socket, &msg, 0);
    if (rc < 0)
    {
        perror("send_tlv");
        debug(D_WRITER, 1, "send_tlv", "envoie non effectué");
        return false;
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

bool_t add_tlv(struct sockaddr_in6 dest, data_t *tlv)
{
    if (tlv == NULL)
    {
        debug(D_WRITER, 1, "add_tlv", "tlv = (null)");
        return false;
    }
    if (g_write_buf == NULL)
    {
        buffer_node_t *node = malloc(sizeof(buffer_node_t));
        if (node == NULL)
        {
            debug(D_WRITER, 1, "add_tlv", "erreur de malloc node");
            return false;
        }
        data_t **tlvs = malloc(sizeof(data_t *) * 1);
        if (tlvs == NULL)
        {
            free(node);
            debug(D_WRITER, 1, "add_tlv", "erreur malloc tlvs");
            return false;
        }
        memset(node, 0, sizeof(buffer_node_t));
        memset(tlvs, 0, sizeof(data_t *) * 1);
        tlvs[0] = copy_iovec(tlv);
        node->dest = dest;
        node->tlvs = tlvs;
        node->tlvlen = 1;
        node->next = NULL;

        g_write_buf = node;
        debug(D_WRITER, 0, "add_tlv", "initialisation première node du buffer");
        return true;
    }
    buffer_node_t *father = g_write_buf;
    buffer_node_t *child = g_write_buf;
    while (child != NULL &&
           (memcmp(&(child->dest).sin6_addr, &dest.sin6_addr, sizeof(dest.sin6_addr)) ||
            (child->dest).sin6_port != dest.sin6_port))
    {
        father = child;
        child = child->next;
    }
    if (child == NULL)
    {
        father->next = NULL;
    }
    return 0;
}