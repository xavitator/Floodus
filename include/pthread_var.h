#ifndef PTHREAD_VAR_H
#define PTHREAD_VAR_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "hashmap.h"

#define D_PTHREAD 1

typedef struct pthread_var_t {
  pthread_mutex_t locker; // Mutex pour locker les threads
  void *content; // La variable contenue
} pthread_var_t;

short lock(pthread_var_t *g_lock);
short unlock(pthread_var_t *g_lock);

hashmap_t *get_hashmap_from(pthread_var_t *h_var);

#endif // PTHREAD_VAR_H
