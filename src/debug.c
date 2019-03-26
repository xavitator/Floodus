#include "debug.h"

/**
 * @brief
 * Message de debug général
 * @param name le nom de la constante ou la chaine vide
 * @param msg le message à afficher
 */
void debug (char * name, const char *msg) {
  if (DEBUG) {
    fprintf(stderr, "[DEBUG] Str %s : %s\n",name, msg);
  }
}

/**
 * @brief
 * Message de debug général et quitte
 * @param name le nom de la chaine
 * @param msg le message
 * @param exit_code le code d'erreur
 */
void debug_and_exit(char * name, const char *msg, int exit_code) {
  if (DEBUG) {
    fprintf(stderr, "[DEBUG] Str %s : %s\n",name, msg);
    exit(exit_code);
  }
}


/**
 * @brief
 * Affiche une requête sous forme hexadécimale
 * @param name le nom de la requête
 * @param data le tableau d'octets
 * @param length la taille des données
 */
void debug_hex(char *name, uint8_t *data, int length) {
  if (DEBUG) {
    fprintf(stderr, "[DEBUG] Hexa %s : ", name);
    for (int i = 0 ; i < length ; i++) {
      fprintf(stderr, "%.2x ", data[i]);
    } 
    fprintf(stderr, "\n");
  }
}

/**
 * @brief
 * Affiche une requête sous forme hexadécimale et quitte
 * @param name le nom de la requête
 * @param data le tableau d'octets
 * @param length la taille des données
 * @param exit_code le code d'erreur
 */
void debug_hex_and_exit (char *name, uint8_t *data, int length, int exit_code) {
  if (DEBUG) {
    fprintf(stderr, "[DEBUG] Hexa %s : ", name);
    for (int i = 0 ; i < length ; i++) {
      fprintf(stderr, "%.2x ", data[i]);
    } 
    fprintf(stderr, "\n");
    exit(exit_code);
  }
}

/**
 * @brief
 * Affiche un int
 * @param name le nom de l'int
 * @param rc l'int à afficher
 */
void debug_int(char *name, int rc) {
  if(DEBUG) {
    fprintf(stderr, "[DEBUG] Int %s: %d\n",name, rc);
  }
}

/**
 * @brief
 * Affiche un int et quitte
 * @param name le nom de l'int
 * @param rc l'int
 * @param exit_code le code d'erreur
 */
void debug_int_and_exit(char *name, int rc, int exit_code) {
  if(DEBUG) {
    fprintf(stderr, "[DEBUG] Int %s : %d\n",name, rc);
    exit(exit_code);
  }
}


