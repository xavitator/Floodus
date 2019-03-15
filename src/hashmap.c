
/**
 * @file hashmap.c Fichier source d'une Hashmap en C.
 * @author Floodus
 * @brief Implémentation d'une hashmap
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include "hashmap.h"

#define false 0
#define true 1

typedef char bool_t;

/**
 * @brief Fonction pour libérer la mémoire d'une struct iovec et de son contenu.
 * 
 * @param data pointeur vers la struct iovec à libérer
 */
void freeiovec(struct iovec *data)
{
    if (data == NULL)
        return;
    free(data->iov_base);
    free(data);
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
        return NULL;
    struct iovec *copy = malloc(sizeof(data));
    if (copy == NULL)
        return NULL;
    void *content = malloc(data->iov_len);
    if (content == NULL)
    {
        free(copy);
        return NULL;
    }
    memcpy(content, data->iov_base, data->iov_len);
    copy->iov_base = content;
    copy->iov_len = data->iov_len;
    return copy;
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
        return 0;
    if (data1 == NULL)
        return -1;
    if (data2 == NULL)
        return 1;
    if (data1->iov_len < data2->iov_len)
        return -1;
    if (data2->iov_len < data1->iov_len)
        return 1;
    return memcmp(data1->iov_base, data2->iov_base, data1->iov_len);
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

/**
 * @brief Fonction de Hachage
 * 
 * @param key clé à hacher
 * @return ssize_t résulat de la fonction de hachage. Il peut être négatif si la fonction de hachage n'a pas pu aboutir.
 */
ssize_t hash(data_t *key)
{
    if (key == NULL)
        return -1;
    u_int16_t res = 0;
    size_t len = key->iov_len;
    if (len == 0)
        return 0;
    size_t nb_block = (len % BIT_MAPSIZE != 0) ? len / BIT_MAPSIZE + 1 : len / BIT_MAPSIZE;
    u_int16_t *block = malloc(nb_block * sizeof(u_int16_t));
    if (block == NULL)
        return -1;
    memset(block, 0, nb_block);
    for (size_t i = 0; i < nb_block; i++)
    {
        size_t taille = (BIT_MAPSIZE > len - BIT_MAPSIZE) ? len - BIT_MAPSIZE : BIT_MAPSIZE;
        block[i] = (1UL << (16 - BIT_MAPSIZE)) - 1;
        block[i] <<= BIT_MAPSIZE; // ajout de 4 bits à 1 au debut du block
        memcpy(&block[i], key->iov_base + i * BIT_MAPSIZE, taille);
    }
    res = block[0];
    for (size_t i = 0; i < nb_block; i++)
    {
        res ^= block[i];
        if (i < nb_block - 1)
            res += block[i + 1];
    }
    free(block);
    return res % HASHMAP_SIZE; // suppression des bits pour que res soit borné entre 0 et HASHMAP_SIZE
}

/**
 * @brief Création d'une hashmap.
 * 
 * @return hashmap_t* Hashmap créée.
 */
hashmap_t *init_map(void)
{
    hashmap_t *map = malloc(sizeof(hashmap_t));
    if (map == NULL)
        return NULL;
    memset(map, 0, sizeof(hashmap_t));
    return map;
}

/**
 * @brief On récupère la valeur associée à 'key'
 * 
 * @param key clé de la valeur recherchée
 * @param map hashmap dans lequel on cherche.
 * @return data_t* renvoie une copie du struct iovec correspondant à la valeur de la key, ou NULL si la key n'existe pas.
 */
data_t *get_map(data_t *key, hashmap_t *map)
{
    if (key == NULL || map == NULL)
        return NULL;
    ssize_t ind = hash(key);
    if (ind < 0)
        return NULL;
    node_t *p = map->content[ind];
    while (p != NULL && compare_iovec(key, p->key) != 0)
        p = p->next;
    if (p == NULL)
        return NULL;
    data_t *value = copy_iovec(p->value);
    if (value == NULL)
        return NULL;
    return value;
}

/**
 * @brief change la valeur de key par value. Si key n'existe pas, on crée la valeur.
 * 
 * @param key clé
 * @param value valeur
 * @param map hashmap dans lequel on fait l'opération.
 * @return char Renvoie 1 si la modification/ajout s'est bien fait, 0 sinon.
 */
bool_t insert_map(data_t *key, data_t *value, hashmap_t *map)
{
    if (key == NULL || map == NULL)
        return false;
    // fin verification des arguments

    data_t *new_key = copy_iovec(key);
    if (new_key == NULL)
        return false;
    data_t *new_value = copy_iovec(value);
    if (new_value == NULL)
    {
        freeiovec(new_key);
        return false;
    }
    // fin initialisation

    ssize_t ind = hash(key);
    if (ind < 0)
        return false;
    node_t *p = map->content[ind];
    node_t *r = p;
    while (p != NULL && compare_iovec(key, p->key) != 0)
    {
        r = p;
        p = p->next;
    }

    // si la node existe deja
    if (p != NULL)
    {
        freeiovec(p->value);
        p->value = new_value;
        freeiovec(new_key);
        return true;
    }

    // si la node n'existe pas
    p = malloc(sizeof(node_t));
    if (p == NULL)
    {
        freeiovec(new_key);
        freeiovec(new_value);
        return false;
    }
    p->key = new_key;
    p->value = new_value;
    p->next = NULL;
    if (r == NULL)
        map->content[ind] = p;
    else
        r->next = p;
    map->size++;
    return true;
}

/**
 * @brief On cherche l'existance de la clé dans le hashmap.
 * 
 * @param key clé
 * @param map hashmap dans lequel on fait la recherche.
 * @return renvoie 1 si la key existe dans map, 0 sinon.
 */
bool_t contains_map(data_t *key, hashmap_t *map)
{
    if (key == NULL || map == NULL)
        return false;
    // fin verification des arguments

    ssize_t ind = hash(key);
    if (ind < 0)
        return false;
    node_t *p = map->content[ind];
    while (p != NULL && compare_iovec(key, p->key) != 0)
        p = p->next;
    return p != NULL;
}

/**
 * @brief On supprime 'key' de 'map'.
 * 
 * @param key clé à supprimer
 * @param map hashmap dans lequel on fait l'opération
 * @return  renvoie 1 si la key a bien été supprimer de map, 0 sinon.
 */
bool_t remove_map(data_t *key, hashmap_t *map)
{
    if (key == NULL || map == NULL)
        return false;
    // fin verification des arguments

    ssize_t ind = hash(key);
    if (ind < 0)
        return false;
    node_t *p = map->content[ind];
    node_t *r = p;
    while (p != NULL && compare_iovec(key, p->key) != 0)
    {
        r = p;
        p = p->next;
    }
    if (p == NULL)
        return false;
    if (p == r)
        map->content[ind] = p->next;
    else
        r->next = p->next;
    freeiovec(p->key);
    freeiovec(p->value);
    free(p);
    map->size--;
    return true;
}

/**
 * @brief On récupère le nombre de clé stockés dans 'map'.
 * 
 * @param map hashmap dont on veut savoir la taille.
 * @return size_t renvoie le nombre d'éléments contenus dans map.
 */
size_t get_size_map(hashmap_t *map)
{
    if (map == NULL)
        return 0;
    return map->size;
}

/**
 * @brief On vérifie si 'map' est vide (ne contient aucun élément).
 * 
 * @param map hashmap
 * @return renvoie 1 si map ne contient aucun élément, 0 sinon.
 */
bool_t empty_map(hashmap_t *map)
{
    return 0 == get_size_map(map);
}

/**
 * @brief vide map de tous ses éléments.
 * 
 * @param map hashmap
 */
void clear_map(hashmap_t *map)
{
    if (map == NULL)
        return;
    node_t **content = map->content;
    for (size_t i = 0; i < HASHMAP_SIZE; i++)
    {
        node_t *r = content[i];
        node_t *p = r;
        while (p != NULL)
        {
            r = p;
            p = p->next;
            freeiovec(r->key);
            freeiovec(r->value);
            free(r);
        }
        content[i] = NULL;
    }
}

/**
 * @brief On libère la mémoire de map et de son contenu.
 * 
 * @param map hashmap
 */
void freehashmap(hashmap_t *map)
{
    clear_map(map);
    free(map);
}

/**
 * @brief Affichage du contenu de la hashmap donnée en argument
 * 
 * @param map hashmap
 */
void print_hashmap(hashmap_t *map)
{
    size_t node = 0;
    if (map == NULL)
    {
        printf("(null)\n");
        return;
    }
    printf("size : %ld\n", map->size);
    node_t **content = map->content;
    for (size_t i = 0; i < HASHMAP_SIZE; i++)
    {
        node_t *p = content[i];
        while (p != NULL)
        {
            node++;
            printf("Node numero %ld\n", node);
            printf("Key : \n");
            print_iovec(p->key);
            printf("Value : \n");
            print_iovec(p->value);
        }
    }
}