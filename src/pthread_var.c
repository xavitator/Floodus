/*
 * @brief
 * Fichier fournissant des fonctions pour
 * récupérer les données d'une pthread_var_t
 *
 * @author Floodus
 */

#include "pthread_var.h"

/**
 * @brief
 * Bloque le mutex de la structure
 *
 * @param g_lock variable contenant le mutex
 */
short lock(pthread_var_t *g_lock) {
  int rc = 0;
  rc = pthread_mutex_lock(&g_lock->locker);
  if(rc)
  {
    debug(D_PTHREAD, 1, "lock -> rc", strerror(rc));
    return rc;
  }
  return rc;
}

/**
 * @brief
 * Débloque le mutex de la structure
 *
 * @param g_lock variable contenant le mutex
 */
short unlock(pthread_var_t *g_lock) {
  int rc = 0;
  rc = pthread_mutex_unlock(&g_lock->locker);
  if(rc)
  {
    debug(D_PTHREAD, 1, "unlock -> rc", strerror(rc));
    return rc;
  }
  return rc;
}


/**
 * @brief
 * Renvoie la hasmap d'une variable
 *
 * @param h_var la structure contenant la hashmap
 */
hashmap_t * get_hasmap_from(pthread_var_t *h_var) {
  return (hashmap_t *) h_var->content;
}

