
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct datagram
{
    u_int8_t magic;
    u_int8_t version;
    u_int16_t body_length;
    struct iovec *body;
} datagram;

u_int64_t id = 0;

/**
 * @brief Instancie l'id du user
 * 
 */
void create_user()
{
    id = rand();
    id <<= 31;
    id += rand();
}

/**
 * @brief Construction tlv hello court
 * 
 * @return struct iovec* contenant un tlv hello court
 */
struct iovec *helloc_tlv()
{
    u_int32_t len = sizeof(u_int8_t) * 2 + sizeof(u_int64_t);
    char *tab = malloc(len);
    memset(tab, 2, 1);
    memset(tab + sizeof(u_int8_t), sizeof(u_int64_t), 1);
    memcpy(tab + sizeof(u_int8_t) * 2, &id, sizeof(u_int64_t));
    struct iovec *blk = malloc(sizeof(struct iovec));
    blk->iov_base = tab;
    blk->iov_len = len;
    return blk;
}

/**
 * @brief Construction tlv hello long
 * 
 * @return struct iovec* contenant un tlv hello long
 */
struct iovec *hellol_tlv(u_int64_t dest)
{
    u_int32_t len = sizeof(u_int8_t) * 2 + sizeof(u_int64_t) + sizeof(u_int64_t);
    char *tab = malloc(len);
    memset(tab, 2, 1);
    memset(tab + sizeof(u_int8_t), sizeof(u_int64_t) * 2, 1);
    memset(tab + sizeof(u_int8_t) * 2, id, sizeof(u_int64_t));
    memset(tab + len - sizeof(u_int64_t), dest, sizeof(u_int64_t));
    struct iovec *blk = malloc(sizeof(struct iovec));
    blk->iov_base = tab;
    blk->iov_len = len;
    return blk;
}

/**
 * @brief Construction d'un tlv data
 * 
 * @param disc contenu du string à envoyer
 * @return struct iovec* contenant un tlv data contenant 'disc'
 */
struct iovec *data_tlv(char *disc)
{
    u_int32_t len = sizeof(u_int8_t) * 2 + sizeof(u_int64_t) + sizeof(u_int32_t) + sizeof(u_int8_t) + strlen(disc);
    char *tab = malloc(len);
    char *type = tab;
    char *length = tab + sizeof(u_int8_t);
    char *sender_id = length + sizeof(u_int8_t);
    char *nounce = sender_id + sizeof(u_int64_t);
    char *type_data = nounce + sizeof(u_int32_t);
    char *data = type_data + sizeof(u_int8_t);
    memset(type, 4, sizeof(u_int8_t));
    memset(length, len - sizeof(u_int8_t) * 2, 1);
    memset(sender_id, id, sizeof(u_int64_t));
    memset(nounce, rand(), sizeof(u_int32_t));
    memset(type_data, 0, sizeof(u_int8_t));
    memcpy(data, disc, strlen(disc));
    struct iovec *blk = malloc(sizeof(struct iovec));
    blk->iov_base = tab;
    blk->iov_len = len;
    return blk;
}

/**
 * @brief Construction datagram avec les tlv en argument
 * 
 * @param tlvs tlvs à ajouter au datagram
 * @param len taille du tableau de tlv
 * @return datagram* datagram créé
 */
datagram *make_datagram(struct iovec *tlvs, u_int16_t len)
{
    datagram *el = malloc(sizeof(datagram));
    el->magic = 93;
    el->version = 2;
    el->body_length = len;
    el->body = tlvs;
    return el;
}

int make_demand(int s, struct addrinfo *p)
{
    datagram *el = make_datagram(helloc_tlv(), 1);
    u_int16_t bodylen = 0;
    for (int i = 0; i < el->body_length; i++)
    {
        bodylen += (el->body + i)->iov_len;
    }
    printf("%d\n",bodylen);
    size_t iovlen = 3 + el->body_length;
    struct iovec *iov = malloc(iovlen * (sizeof(struct iovec)));
    struct iovec magic = {0};
    magic.iov_len = sizeof(u_int8_t);
    magic.iov_base = &el->magic;
    struct iovec version = {0};
    version.iov_len = sizeof(u_int8_t);
    version.iov_base = &el->version;
    struct iovec body_length = {0};
    body_length.iov_len = sizeof(u_int16_t);
    u_int16_t tmp = htons(bodylen);
   body_length.iov_base = &tmp;
    iov[0] = magic;
    iov[1] = version;
    iov[2] = body_length;
    for (int i = 0; i < el->body_length; i++)
    {
        el->body[i].iov_len = el->body[i].iov_len;
        iov[i + 3] = el->body[i];
    }
    int rc = 0;

    // envoie avec sendmsg
    struct msghdr msg = {0};
    msg.msg_name = p->ai_addr;
    msg.msg_namelen = p->ai_addrlen;
    msg.msg_iov = iov;
    msg.msg_iovlen = iovlen;
    rc = sendmsg(s, &msg, 0);

    // envoie avec sendto
    char res[4096];
    size_t size = 0;
    for (unsigned int i = 0; i < iovlen; i++)
    {
        memcpy(res + size, iov[i].iov_base, iov[i].iov_len);
        size += iov[i].iov_len;
    }
    for (size_t i = 0; i < size; i++)
    {
        printf("%.2x ", res[i]);
    }
    //rc = sendto(s, res, size, 0, p->ai_addr, p->ai_addrlen);
    if (rc < 0)
        perror("error : ");
    printf("\n%d\n", rc);

    unsigned char req[4096];
    struct iovec io;
    io.iov_base = req;
    io.iov_len = 4096;

    msg.msg_iovlen = 1;
    msg.msg_iov = &io;
    printf("before recvfrom\n");
    //rc = read(s, req, 4096);
    rc = recvmsg(s, &msg, 0);
    printf("test reussi : %d\n", rc);
    return 0;
}

int send_hello()
{
    struct addrinfo h = {0};
    struct addrinfo *r = {0};
    int rc = 0;
    h.ai_family = AF_INET6;
    h.ai_socktype = SOCK_DGRAM;
    h.ai_flags = AI_V4MAPPED | AI_ALL;
    rc = getaddrinfo("jch.irif.fr", "1212", &h, &r);
    if (rc < 0)
    {
        fprintf(stderr, "Error : %s\n", gai_strerror(rc));
        exit(1);
    }
    struct addrinfo *p = r;
    int s = -1;
    rc = -1;
    while (p != NULL)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        int val;
        setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &val, sizeof(val));
        if (s >= 0)
        {
            rc = connect(s, p->ai_addr, p->ai_addrlen);
            if (rc >= 0)
                break;
            close(s);
        }
        p = p->ai_next;
    }
    if (rc < 0)
    {
        printf("Connexion impossible\n");
        exit(1);
    }
    char ip[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, p->ai_addr, ip, INET6_ADDRSTRLEN);
    printf("%s\n", ip);
    create_user();
    make_demand(s, p);
    printf("demande effectuée\n");
    //recv_demand(s, p);
    return 0;
}

int main()
{
    send_hello();
    return 0;
}
