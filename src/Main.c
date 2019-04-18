/**
 * @file Main.c
 * @author Floodus
 * @brief Fichier s'occupant de l'exécutation du programme
 * 
 */

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
#include <signal.h>

#include "TLV.h"
#include "debug.h"
#include "voisin.h"
#include "writer.h"
#include "reader.h"
#include "send_thread.h"
#include "controller.h"

#define D_MAIN 1

/**
 * @brief
 * En cas de ctrl+c stoppe l'ensemble du
 * programme et free les structures.
 */
static void sig_int(int sig)
{
    if (sig == SIGINT)
    {
        stop_program();
    }
}

/**
 * @brief Fonction initialisant tout.
 * 
 */
static void initializer(void)
{
    init_sender();
    init_neighbours();
    signal(SIGINT, sig_int);
}

/**
 * @brief Fonction faisant le ménage à la fin du programme.
 * 
 */
static void finisher(void)
{
    free_neighbours();
    destroy_thread();
    free_inondation();
    free_writer();
}

/**
 * @brief Envoie de hello court à un destinataire contenu dans un addrinfo
 * 
 * @param p destinataire
 * @return int boolean disant si tout s'est bien passé
 */
int make_demand(struct addrinfo *p)
{
    data_t hs = {0};
    if (!hello_short(&hs, g_myid))
    {
        debug(D_MAIN, 1, "make_demand -> new_neighbour", "hs erreur");
        return 0;
    }
    ip_port_t ipport = {0};
    ipport.port = ((struct sockaddr_in6 *)p->ai_addr)->sin6_port;
    memmove(ipport.ipv6, &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr, sizeof(ipport.ipv6));
    int rc = send_tlv(ipport, &hs, 1);

    data_t new_neighbour = {0};
    if (!neighbour(&new_neighbour, ipport.ipv6, ipport.port))
    {
        debug(D_MAIN, 1, "make_demand -> new_neighbour", " new = NULL");
        free(hs.iov_base);
        return 0;
    }
    size_t head = 1;
    rc = apply_tlv_neighbour(&new_neighbour, &head);
    if (rc == false)
        debug(D_MAIN, 1, "make_demand -> apply neighbour", " rc = false");

    free(hs.iov_base);
    free(new_neighbour.iov_base);
    return rc;
}

/**
 * @brief On récupère toutes les infos via getaddrinfo sur la destination et le port passés en arguments.
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
            perror("Main -> Erreur de création de socket ");
        if (rc == -2)
            perror("Main -> Erreur de bind ");
        if (rc == -3)
            perror("Main -> Erreur de récupération des informations ");
        if (rc == -4)
            perror("Main -> Modification des modes de la socket impossible ");
        printf("Main : Problème de connexion");
        exit(1);
    }
    initializer();
    rc = send_hello(default_dest, port);
    if (rc >= 0)
        launch_program();
    finisher();
    return 0;
}
