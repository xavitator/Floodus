#include "view.h"

void print_data(u_int8_t *content, u_int8_t contentlen)
{
    printf("\e[1;34m[DATA]\e[0mAffichage du contenu :\n");
    for (u_int8_t i = 0; i < contentlen; i++)
    {
        printf("%c", content[i]);
    }
    printf("\n");
}