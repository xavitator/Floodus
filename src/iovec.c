#include "iovec.h"

/**
 * @brief Fonction pour libérer la mémoire d'une struct iovec et de son contenu.
 * 
 * @param data pointeur vers la struct iovec à libérer
 */
void freeiovec(struct iovec *data)
{
    if (data == NULL)
    {

        debug(D_IOVEC, 1, "freeiovec", "data : (null)");
        return;
    }
    free(data->iov_base);
    free(data);
    debug(D_IOVEC, 0, "freeiovec", "libération de la mémoire");
}

/**
 * @brief On crée une copie de la struct iovec et de son contenu donnée en paramêtre
 * 
 * @param data struct iovec à copier
 * @return struct iovec* copie du paramêtre
 */
struct iovec *copy_iovec(struct iovec *data)
{
    if (data == NULL)
    {
        debug(D_IOVEC, 1, "copy_iovec", "data : (null)");
        return NULL;
    }
    struct iovec *copy = malloc(sizeof(data));
    if (copy == NULL)
    {
        debug(D_IOVEC, 1, "copy_iovec", "erreur de malloc de la struct iovec");
        return NULL;
    }
    void *content = malloc(data->iov_len);
    if (content == NULL)
    {
        debug(D_IOVEC, 1, "copy_iovec", "erreur de malloc du contenu");
        free(copy);
        return NULL;
    }
    memcpy(content, data->iov_base, data->iov_len);
    copy->iov_base = content;
    copy->iov_len = data->iov_len;
    debug(D_IOVEC, 0, "copy_iovec", "renvoie de la copie");
    return copy;
}

/**
 * @brief Creer une structure iovec contenant une copie de ses arguments.
 * 
 * @param data contenu de la struct iovec
 * @param len taille du contenu
 * @return struct iovec* renvoie NULL si on a un problème de malloc, une struct iovec contenant les arguments sinon.
 */
struct iovec *create_iovec(void *data, size_t len)
{
    struct iovec *create = malloc(sizeof(struct iovec));
    memset(create, 0, sizeof(struct iovec));
    if (create == NULL)
    {
        debug(D_IOVEC, 1, "create_iovec", "problème de malloc pour create");
        return NULL;
    }
    void *content = malloc(len);
    if (content == NULL)
    {
        debug(D_IOVEC, 1, "create_iovec", "problème de malloc pour content");
        free(create);
        return NULL;
    }
    memmove(content, data, len);
    create->iov_base = content;
    create->iov_len = len;
    debug(D_IOVEC, 0, "create_iovec", "création de la struct iovec");
    return create;
}

/**
 * @brief Fonction de comparaison du contenu de deux struct iovec.
 * 
 * @param data1 donnée 1
 * @param data2 donnée 2
 * @return int Renvoie un entier inférieur, égal, ou supérieur à zéro, si data1 est respectivement inférieure, égale ou supérieur à data2.  
 */
int compare_iovec(struct iovec *data1, struct iovec *data2)
{
    if (data1 == NULL && data2 == NULL)
    {
        debug_int(D_IOVEC, 0, "compare_iovec : deux structures NULL", 0);
        return 0;
    }
    if (data1 == NULL)
    {
        debug_int(D_IOVEC, 0, "compare_iovec : data1 NULL", -1);
        return -1;
    }
    if (data2 == NULL)
    {
        debug_int(D_IOVEC, 0, "compare_iovec : data2 NULL", 1);
        return 1;
    }
    if (data1->iov_len < data2->iov_len)
    {
        debug_int(D_IOVEC, 0, "compare_iovec : taille de data1 inférieure", -1);
        return -1;
    }
    if (data2->iov_len < data1->iov_len)
    {
        debug_int(D_IOVEC, 0, "compare_iovec : taille de data2 inféreieure", 1);
        return 1;
    }
    int res = memcmp(data1->iov_base, data2->iov_base, data1->iov_len);
    debug_int(D_IOVEC, 0, "compare_iovec : comparaison de bit", res);
    return res;
}

/**
 * @brief Afficher le contenu d'une struct iovec.
 * 
 * @param data Struct iovec à afficher.
 */
void print_iovec(struct iovec *data)
{
    printf("Size iovec : %ld\nContent : ", data->iov_len);
    for (size_t i = 0; i < data->iov_len; i++)
    {
        printf("%.2x ", ((u_int8_t *)data->iov_base)[i]);
    }
    printf("\n");
}