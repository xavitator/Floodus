
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

#include "TLV.h"
#include "debug.h"
#include "voisin.h"
#include "writer.h"
#include "reader.h"

#define D_MAIN 1

int make_demand(int s, struct addrinfo *p)
{
    init_writer(s);
    data_t *hs = hello_short(myid);
    ip_port_t ipport = {0};
    memmove(&ipport.port, &((struct sockaddr_in6 *)p->ai_addr)->sin6_port, sizeof(ipport.port));
    memmove(ipport.ipv6, &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr, sizeof(ipport.ipv6));
    send_tlv(&ipport, hs, 1);

    // struct sockaddr test = {0};
    // socklen_t testlen = sizeof(test);
    printf("before recvfrom\n");
    //rc = read(s, req, 4096);
    // unsigned char req[4096];
    ssize_t rc = read_msg();
    //rc = recvmsg(s, &msg, 0);
    debug_int(D_MAIN, 0, "rc after test", *(int *)&rc);
    // debug_int(D_MAIN, 0, "msg length", msg.msg_iovlen);
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
        debug_and_exit(D_MAIN, 1, "rc", gai_strerror(rc), 1);
    struct addrinfo *p = r;
    int s = -1;
    while (p != NULL)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
            break;
        p = p->ai_next;
    }
    if (s < 0 || p == NULL)
    {
        freeaddrinfo(r);
        debug_and_exit(D_MAIN, 1, "p", "Connexion impossible", 1);
    }
    char ip[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, p->ai_addr, ip, INET6_ADDRSTRLEN);
    debug(D_MAIN, 0, "ip", ip);
    make_demand(s, p);
    printf("demande effectuée\n");
    //recv_demand(s, p);
    return 0;
}

int main()
{
    init_neighbors();
    send_hello();
    free_neighbors();
    return 0;
}
