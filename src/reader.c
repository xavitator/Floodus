#include "reader.h"

typedef bool_t (*tlv_function_t)(ip_port_t, data_t *, size_t *);

bool_t tlv_call_pad1(ip_port_t dest, data_t *data, size_t *head_read)
{
    debug_hex(D_READER, 0, "", (u_int8_t *)&dest, sizeof(dest));
    if (((u_int8_t *)data->iov_base)[*head_read] != 0)
    {
        debug(D_READER, 1, "tlv_call_pad1", "mauvais type");
        return false;
    }
    *head_read += 1;
    debug(D_READER, 0, "tlv_call_pad1", "lecture du pad1");
    return true;
}

bool_t tlv_call_padn(ip_port_t dest, data_t *data, size_t *head_read)
{
    debug_hex(D_READER, 0, "", (u_int8_t *)&dest, sizeof(dest));
    if (((u_int8_t *)data->iov_base)[*head_read] != 1)
    {
        debug(D_READER, 1, "tlv_call_padn", "mauvais type");
        return false;
    }
    *head_read += 1;
    u_int8_t len = ((u_int8_t *)data->iov_base)[*head_read];
    if (*head_read + len > data->iov_len)
    {
        debug(D_READER, 1, "tlv_call_padn", "taille du message non correspondante");
        return false;
    }
    *head_read += len;
    debug(D_READER, 0, "tlv_call_padn", "lecture du padn");
    return true;
}

bool_t tlv_call_hello(ip_port_t dest, data_t *data, size_t *head_read)
{
    debug_hex(D_READER, 0, "", (u_int8_t *)&dest, sizeof(dest));
    if (((u_int8_t *)data->iov_base)[*head_read] != 2)
    {
        debug(D_READER, 1, "tlv_call_hello", "mauvais type");
        return false;
    }
    *head_read += 1;
    u_int8_t len = ((u_int8_t *)data->iov_base)[*head_read];
    if (*head_read + len > data->iov_len)
    {
        debug(D_READER, 1, "tlv_call_hello", "taille du message non correspondante");
        return false;
    }
    bool_t res = apply_tlv_hello(dest, data, head_read);
    if (res == false)
    {
        debug(D_READER, 1, "tlv_call_hello", "problème application du tlv hello");
        return res;
    }
    debug(D_READER, 0, "tlv_call_hello", "traitement du tlv hello effectué");
    return true;
}

bool_t tlv_call_neighbour(ip_port_t dest, data_t *data, size_t *head_read)
{
    debug_hex(D_READER, 0, "", (u_int8_t *)&dest, sizeof(dest));
    if (((u_int8_t *)data->iov_base)[*head_read] != 3)
    {
        debug(D_READER, 1, "tlv_call_neighbour", "mauvais type");
        return false;
    }
    *head_read += 1;
    u_int8_t len = ((u_int8_t *)data->iov_base)[*head_read];
    if (*head_read + len > data->iov_len)
    {
        debug(D_READER, 1, "tlv_call_neighbour", "taille du message non correspondante");
        return false;
    }
    bool_t res = apply_tlv_neighbour(data, head_read);
    if (res == false)
    {
        debug(D_READER, 1, "tlv_call_neighbour", "problème application du tlv neighbour");
        return res;
    }
    debug(D_READER, 0, "tlv_call_neighbour", "traitement du tlv neighbour effectué");
    return true;
}

bool_t tlv_call_data(ip_port_t dest, data_t *data, size_t *head_read)
{
    debug_hex(D_READER, 0, "", (u_int8_t *)&dest, sizeof(dest));
    if (((u_int8_t *)data->iov_base)[*head_read] != 4)
    {
        debug(D_READER, 1, "tlv_call_data", "mauvais type");
        return false;
    }
    *head_read += 1;
    u_int8_t len = ((u_int8_t *)data->iov_base)[*head_read];
    if (*head_read + len > data->iov_len)
    {
        debug(D_READER, 1, "tlv_call_data", "taille du message non correspondante");
        return false;
    }
    debug_hex(D_WRITER, 0, "tlv_call_data", (uint8_t *)data->iov_base + *head_read + 1, len);
    debug(D_READER, 0, "tlv_call_data", "traitement du tlv data en cours");
    return true;
}

bool_t tlv_call_ack(ip_port_t dest, data_t *data, size_t *head_read)
{
    debug_hex(D_READER, 0, "", (u_int8_t *)&dest, sizeof(dest));
    if (((u_int8_t *)data->iov_base)[*head_read] != 5)
    {
        debug(D_READER, 1, "tlv_call_ack", "mauvais type");
        return false;
    }
    *head_read += 1;
    u_int8_t len = ((u_int8_t *)data->iov_base)[*head_read];
    if (*head_read + len > data->iov_len)
    {
        debug(D_READER, 1, "tlv_call_ack", "taille du message non correspondante");
        return false;
    }
    debug_hex(D_WRITER, 0, "tlv_call_ack", (uint8_t *)data->iov_base + *head_read + 1, len);
    debug(D_READER, 0, "tlv_call_ack", "traitement du tlv ack en cours");
    return true;
}

bool_t tlv_call_goaway(ip_port_t dest, data_t *data, size_t *head_read)
{
    debug_hex(D_READER, 0, "", (u_int8_t *)&dest, sizeof(dest));
    if (((u_int8_t *)data->iov_base)[*head_read] != 6)
    {
        debug(D_READER, 1, "tlv_call_goaway", "mauvais type");
        return false;
    }
    *head_read += 1;
    u_int8_t len = ((u_int8_t *)data->iov_base)[*head_read];
    if (*head_read + len > data->iov_len)
    {
        debug(D_READER, 1, "tlv_call_goaway", "taille du message non correspondante");
        return false;
    }
    debug_hex(D_WRITER, 0, "tlv_call_goaway", (uint8_t *)data->iov_base + *head_read + 1, len);
    debug(D_READER, 0, "tlv_call_goaway", "traitement du tlv goaway en cours");
    return true;
}

bool_t tlv_call_warning(ip_port_t dest, data_t *data, size_t *head_read)
{
    debug_hex(D_READER, 0, "", (u_int8_t *)&dest, sizeof(dest));
    if (((u_int8_t *)data->iov_base)[*head_read] != 7)
    {
        debug(D_READER, 1, "tlv_call_warning", "mauvais type");
        return false;
    }
    *head_read += 1;
    u_int8_t len = ((u_int8_t *)data->iov_base)[*head_read];
    if (*head_read + len > data->iov_len)
    {
        debug(D_READER, 1, "tlv_call_warning", "taille du message non correspondante");
        return false;
    }
    char content[len + 1];
    memmove(content, data->iov_base + *head_read + 1, len);
    content[len] = '\0';
    debug(D_WRITER, 0, "tlv_call_warning -> message", content);
    debug(D_READER, 0, "tlv_call_warning", "traitement du tlv warning en cours");
    return true;
}

tlv_function_t tlv_function_call[NB_TLV] = {

    tlv_call_pad1,
    tlv_call_padn,
    tlv_call_hello,
    tlv_call_neighbour,
    tlv_call_data,
    tlv_call_ack,
    tlv_call_goaway,
    tlv_call_warning

};

void read_tlv(ip_port_t dest, data_t *tlvs)
{
    size_t head_reader = 0;
    u_int8_t type = 0;
    while (head_reader < tlvs->iov_len)
    {
        type = ((u_int8_t *)tlvs->iov_base)[head_reader];
        if (type >= NB_TLV)
        {
            debug_int(D_READER, 0, "read_tlv -> type inconnu de tlv", type);
            head_reader += 1;
            if (head_reader + 1 >= tlvs->iov_len)
                break;
            head_reader += ((u_int8_t *)tlvs->iov_base)[head_reader];
            continue;
        }
        debug_int(D_READER, 0, "read_tlv -> commencement du traitement du tlv", type);
        tlv_function_call[type](dest, tlvs, &head_reader);
    }
    debug(D_READER, 0, "read_tlv", "fin de lecture du tlv");
}

ssize_t read_msg()
{
    struct sockaddr_in6 recv = {0};
    socklen_t recvlen = sizeof(recv);
    uint8_t header[RDHDRLEN] = {0};
    struct iovec header_iovec = {0};
    header_iovec.iov_base = header;
    header_iovec.iov_len = RDHDRLEN;
    uint8_t req[READBUF] = {0};
    struct iovec corpus = {0};
    corpus.iov_base = req;
    corpus.iov_len = READBUF;
    struct msghdr reader = {0};
    reader.msg_name = &recv;
    reader.msg_namelen = recvlen;
    data_t content[2] = {header_iovec, corpus};
    reader.msg_iov = content;
    reader.msg_iovlen = 2;
    ssize_t rc = 0;
    while (1)
    {
        rc = recvmsg(g_socket, &reader, 0);
        if (rc >= 0)
            break;
        if (errno == EINTR ||
            (errno != EWOULDBLOCK && errno != EAGAIN))
        {
            debug(D_WRITER, 1, "read_msg -> reception non effectué", strerror(errno));
            return -1;
        }
    }
    header_iovec = content[0];
    uint8_t expected[] = {93, 2};
    if (memcmp(header_iovec.iov_base, expected, 2) != 0)
    {
        debug(D_READER, 0, "read_msg", "les champs magic et version ne correspondent pas");
        return 0;
    }
    uint16_t len_msg = 0;
    memmove(&len_msg, ((uint8_t *)header_iovec.iov_base) + 2, 2);
    len_msg = ntohs(len_msg);
    printf("%d debug\n", len_msg);
    if (len_msg != rc - RDHDRLEN)
    {
        debug(D_READER, 1, "read_msg", "taille lue et taille attendue différente");
        return 0;
    }
    ip_port_t ipport = {0};
    memmove(&ipport.port, &((struct sockaddr_in6 *)reader.msg_name)->sin6_port, sizeof(ipport.port));
    memmove(ipport.ipv6, &((struct sockaddr_in6 *)reader.msg_name)->sin6_addr, sizeof(ipport.ipv6));
    content[1].iov_len = len_msg;
    read_tlv(ipport, &content[1]);
    debug(D_WRITER, 0, "read_msg", "lecture d'un datagramme");
    return rc;
}