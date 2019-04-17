/**
 * @file debug.c
 * @author Floodus
 * @brief Module implémentant toutes les fonctions de debug
 * 
 */

#include "debug.h"

/**
 * @brief
 * Message de debug général
 * @param flag flag de debug de la fonction appelante
 * @param error permet de marquer une erreur
 * @param name le nom de la constante ou la chaine vide
 * @param msg le message à afficher
 */
void debug(uint8_t flag, uint8_t error, char *name, const char *msg)
{
  char *d = (error) ? "\e[1;31m[DEBUG]\e[0m" : "\e[1;32m[DEBUG]\e[0m";
  if (DEBUG && flag)
  {
    fprintf(stderr, "%s Str %s : %s\n", d, name, msg);
  }
}

/**
 * @brief
 * Message de debug général et quitte
 * @param flag flag de debug de la fonction appelante
 * @param error permet de marquer une erreur
 * @param name le nom de la chaine
 * @param msg le message
 * @param exit_code le code d'erreur
 */
void debug_and_exit(uint8_t flag, uint8_t error, char *name, const char *msg, int exit_code)
{
  char *d = (error) ? "\e[1;31m[DEBUG]\e[0m" : "\e[1;32m[DEBUG]\e[0m";
  if (DEBUG && flag)
  {
    fprintf(stderr, "%s Str %s : %s\n", d, name, msg);
    exit(exit_code);
  }
}

/**
 * @brief
 * Affiche une requête sous forme hexadécimale
 * @param flag flag de debug de la fonction appelante
 * @param error permet de marquer une erreur
 * @param name le nom de la requête
 * @param data le tableau d'octets
 * @param data_len la taille des données
 */
void debug_hex(uint8_t flag, uint8_t error, char *name, void *data, int data_len)
{
  char *d = (error) ? "\e[1;31m[DEBUG]\e[0m" : "\e[1;32m[DEBUG]\e[0m";
  if (DEBUG && flag)
  {
    fprintf(stderr, "%s Hexa %s : ", d, name);
    for (int i = 0; i < data_len; i++)
    {
      fprintf(stderr, "%.2x ", ((uint8_t *)data)[i]);
    }
    fprintf(stderr, "\n");
  }
}

/**
 * @brief
 * Affiche une requête sous forme hexadécimale et quitte
 * @param flag flag de debug de la fonction appelante
 * @param error permet de marquer une erreur 
 * @param name le nom de la requête
 * @param data le tableau d'octets
 * @param data_len la taille des données
 * @param exit_code le code d'erreur
 */
void debug_hex_and_exit(uint8_t flag, uint8_t error, char *name, void *data, int data_len, int exit_code)
{
  char *d = (error) ? "\e[1;31m[DEBUG]\e[0m" : "\e[1;32m[DEBUG]\e[0m";
  if (DEBUG && flag)
  {
    fprintf(stderr, "%s Hexa %s : ", d, name);
    for (int i = 0; i < data_len; i++)
    {
      fprintf(stderr, "%.2x ", ((uint8_t *)data)[i]);
    }
    fprintf(stderr, "\n");
    exit(exit_code);
  }
}

/**
 * @brief
 * Affiche un int
 * @param flag flag de debug de la fonction appelante
 * @param error permet de marquer une erreur
 * @param name le nom de l'int
 * @param rc l'int à afficher
 */
void debug_int(uint8_t flag, uint8_t error, char *name, int rc)
{
  char *d = (error) ? "\e[1;31m[DEBUG]\e[0m" : "\e[1;32m[DEBUG]\e[0m";
  if (DEBUG && flag)
  {
    fprintf(stderr, "%s Int %s: %d\n", d, name, rc);
  }
}

/**
 * @brief
 * Affiche un int et quitte
 * @param flag flag de debug de la fonction appelante
 * @param error permet de marquer une erreur
 * @param name le nom de l'int
 * @param rc l'int
 * @param exit_code le code d'erreur
 */
void debug_int_and_exit(uint8_t flag, uint8_t error, char *name, int rc, int exit_code)
{
  char *d = (error) ? "\e[1;31m[DEBUG]\e[0m" : "\e[1;32m[DEBUG]\e[0m";
  if (DEBUG && flag)
  {
    fprintf(stderr, "%s Int %s : %d\n", d, name, rc);
    exit(exit_code);
  }
}
