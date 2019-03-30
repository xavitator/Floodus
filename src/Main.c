
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

/**
 * @brief Envoie de hello court à un destinataire contenu dans un addrinfo
 * 
 * @param p destinataire
 * @return int boolean disant si tout s'est bien passé
 */
int make_demand(struct addrinfo *p)
{
    data_t *hs = hello_short(myid);
    ip_port_t ipport = {0};
    ipport.port = ((struct sockaddr_in6 *)p->ai_addr)->sin6_port;
    memmove(ipport.ipv6, &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr, sizeof(ipport.ipv6));
    int rc = send_tlv(&ipport, hs, 1);
    freeiovec(hs);
    return rc;
}

/**
 * @brief On récupère toutes les infos via getaddrinfo sur la destination et le port passé en arguments.
 * 
 * @param dest nom dns de la destination
 * @param port port de la destination
 * @return int '0' si ca s'est bien passé, '-1' sinon.
 */
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
    {
        debug(D_MAIN, 1, "send_hello -> rc", gai_strerror(rc));
        return -1;
    }
    struct addrinfo *p = r;

    // demande à toutes les interfaces détectées
    // while (p != NULL)
    // {
    //     make_demand(p);
    //     p = p->ai_next;
    // }
    // fin de la demande à toutes les interfaces

    // demande à la première interface
    if (p == NULL)
    {
        debug(D_MAIN, 1, "send_hello", "aucune interface détectée pour cette adresse");
        return -1;
    }
    make_demand(p);
    // fin de la demande à la première interface

    freeaddrinfo(r);
    debug(D_MAIN, 0, "send_hello", "demande effectuée pour getaddrinfo");
    return 0;
}

/**
 * @brief initialisation du serveur.
 * Le commande de lancement peut prendre 2 arguments.
 * Si les deux arguments sont présents simultanémant :
 * le premier argument correspondra au nom dns de la destination
 * le deuxième argument correspondra au port de la destination
 * 
 * @param argc nombre d'arguments de la commande
 * @param argv tableau des arguments de la commande
 * @return int valeur de retour
 */
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
    rc = send_hello(default_dest, port);
    if (rc >= 0)
        launch_program();
    free_neighbors();
    return 0;
}
