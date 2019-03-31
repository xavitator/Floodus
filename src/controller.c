/**
 * @file controller.c
 * @author Floodus
 * @brief Module s'occupant de faire la liaison entre le reader, le writer et l'inondation
 * 
 * 
 */

#include "controller.h"

/**
 * @brief Variable globale correspodant à une socket
 * 
 */
u_int32_t g_socket = 1;

/**
 * @brief Création de la socket d'écriture et de lecture.
 * On s'occupe de rendre la socket non-bloquante.
 * 
 * @param port port pour le bind
 * @return int '0' si l'opération réussie, un nombre négatif sinon (dépend du type d'erreur).
 */
int create_socket(uint16_t port)
{
    int rc = 0;
    int s = socket(AF_INET6, SOCK_DGRAM, 0);
    if (s < 0)
    {
        rc = errno;
        debug(D_CONTROL, 1, "create_socket -> erreur de création de socket", strerror(errno));
        errno = rc;
        return -1;
    }
    struct sockaddr_in6 server = {0};
    server.sin6_family = AF_INET6;
    server.sin6_port = htons(port);
    socklen_t serverlen = sizeof(server);
    rc = bind(s, (struct sockaddr *)&server, serverlen);
    if (rc < 0)
    {
        rc = errno;
        debug(D_CONTROL, 1, "create_socket -> erreur de bind", strerror(errno));
        errno = rc;
        return -2;
    }
    rc = getsockname(s, (struct sockaddr *)&server, &serverlen);
    if (rc < 0)
    {
        rc = errno;
        debug(D_CONTROL, 1, "create_socket -> erreur getsockname", strerror(errno));
        errno = rc;
        return -3;
    }
    debug_int(D_CONTROL, 0, "create_socket -> port", ntohs(server.sin6_port));
    rc = fcntl(s, F_GETFL);
    if (rc < 0)
    {
        debug(D_CONTROL, 1, "create_socket", "recupération des modes de la socket impossible");
        return -4;
    }
    rc = fcntl(s, F_SETFL, rc | O_NONBLOCK);
    if (rc < 0)
    {
        debug(D_CONTROL, 1, "create_socket", "changement des modes de la socket impossible");
        return -4;
    }
    g_socket = s;
    return 0;
}

/**
 * @brief On récupère le temps qu'il reste avant le prochain message à inonder
 * 
 * @param tm struct timespec à remplir
 * @return bool_t '1' si 'tm' a été rempli, '0' sinon.
 */
bool_t get_nexttime(struct timespec *tm)
{
    tm->tv_nsec = 0;
    tm->tv_sec = 10;
    return 1;
}

void launch_program()
{
    int rc = 0;
    fd_set readfds;
    fd_set writefds;
    struct timespec tm = {0};
    while (1)
    {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(g_socket, &readfds);
        if (buffer_is_empty() == false)
        {
            debug(D_CONTROL, 0, "launch_program", "ajout de la socket g_socket en écriture");
            FD_SET(g_socket, &writefds);
        }
        get_nexttime(&tm);
        if (pselect(g_socket + 1, &readfds, &writefds, NULL, &tm, NULL) > 0)
        {
            if (FD_ISSET(g_socket, &readfds))
            {
                rc = (int)read_msg();
                if (rc < 0)
                {
                    debug(D_CONTROL, 1, "launch_program", "message non lu -> lancer debug reader pour savoir");
                }
                else
                    debug_int(D_CONTROL, 0, "launch_program -> taille lue et interprétée", rc);
            }
            if (FD_ISSET(g_socket, &writefds))
            {
                rc = send_buffer_tlv();
                if (rc == false)
                {
                    debug(D_CONTROL, 1, "launch_program", "message non envoyé");
                }
                else
                    debug(D_CONTROL, 0, "launch_program", "envoie d'un message");
            }
        }
    }
}
