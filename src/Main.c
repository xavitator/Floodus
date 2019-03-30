
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
#include "send_thread.h"
#include "controller.h"

#define D_MAIN 1

int make_demand(struct addrinfo *p)
{
    data_t *hs = hello_short(myid);
    ip_port_t ipport = {0};
    ipport.port = ((struct sockaddr_in6 *)p->ai_addr)->sin6_port;
    memmove(ipport.ipv6, &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr, sizeof(ipport.ipv6));
    send_tlv(&ipport, hs, 1);
    freeiovec(hs);
    return 0;
}

int send_hello(char *dest, char *port)
{
    struct addrinfo h = {0};
    struct addrinfo *r = {0};
    int rc = 0;
    h.ai_family = AF_INET6;
    h.ai_socktype = SOCK_DGRAM;
    h.ai_flags = AI_V4MAPPED | AI_ALL;
    rc = getaddrinfo(dest, port, &h, &r);
    if (rc < 0)
        debug_and_exit(D_MAIN, 1, "rc", gai_strerror(rc), 1);
    struct addrinfo *p = r;
    while (p != NULL)
    {
        make_demand(p);
        p = p->ai_next;
    }
    debug(D_MAIN, 0, "send_hello", "demande effectuée pour getaddrinfo");
    return 0;
}

int main(int argc, char *argv[])
{
    char *port = "1212";
    char *default_dest = "jch.irif.fr";
    if (argc >= 3)
    {
        default_dest = argv[1];
        port = argv[2];
    }
    printf("%s - %s - %s\n", argv[0], default_dest, port);
    int rc = create_socket(0);
    if (rc < 0)
    {
        if (rc == -1)
            perror("Erreur de création de socket");
        if (rc == -2)
            perror("Erreur de bind");
        if (rc == -3)
            perror("Erreur de récupération des informations");
        if (rc == -4)
            perror("Modification des modes de la socket impossible");
        printf("Problème de connexion");
        exit(1);
    }
    init_sender();
    init_neighbors();
    send_hello(default_dest, port);
    launch_program();
    free_neighbors();
    return 0;
}
