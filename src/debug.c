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
  if(error)
    set_in_red();
  else
    set_in_green();
  if (DEBUG && flag)
    wprintw(get_panel(), "[Debug] Str %s : %s\n", name, msg);
  restore();
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
  if(error)
    set_in_red();
  else
    set_in_green();
  if (DEBUG && flag)
  {
    wprintw(get_panel(), "[Debug] Str %s : %s\n", name, msg);
    restore();
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
  if(error)
    set_in_red();
  else
    set_in_green();

  if (DEBUG && flag)
  {
    wprintw(get_panel(), "[Debug] Hexa %s : ", name);
    for (int i = 0; i < data_len; i++)
    {
      wprintw(get_panel(), "%.2x ", ((uint8_t *)data)[i]);
    }
    wprintw(get_panel(), "\n");
  }
  restore();
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
  if(error)
    set_in_red();
  else
    set_in_green();
  
  if (DEBUG && flag)
  {
    wprintw(get_panel(), "[Debug] Hexa %s : ", name);
    for (int i = 0; i < data_len; i++)
    {
      wprintw(get_panel(), "%.2x ", ((uint8_t *)data)[i]);
    }
    wprintw(get_panel(), "\n");
    restore();
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
  if(error)
    set_in_red();
  else
    set_in_green();
  
  if (DEBUG && flag)
  {
    wprintw(get_panel(), "[Debug] Int %s: %d\n", name, rc);
  }
  restore();
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
  if(error)
    set_in_red();
  else
    set_in_green();
  
  if (DEBUG && flag)
  {
    wprintw(get_panel(), "%s Int %s : %d\n", name, rc);
    restore();
    exit(exit_code);
  }
}
