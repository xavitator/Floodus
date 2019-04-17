#include "view.h"

/**
 * @brief Fonction d'affichage d'un message donnée en argument. Cette fonction n'utilise pas de fonctions de debug.
 * 
 * @param content contenu du message (peut ne pas se terminer par '\0')
 * @param content_len taille du contenu
 */
void print_data(u_int8_t *content, u_int8_t content_len)
{
    printf("\e[1;34m[DATA]\e[0mAffichage du contenu :\n");
    for (u_int8_t i = 0; i < content_len; i++)
    {
        printf("%c", content[i]);
    }
    printf("\n");
}