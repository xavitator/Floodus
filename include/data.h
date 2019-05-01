#ifndef _DATA_H
#define _DATA_H

#include "hashmap.h"
#include "view.h"
#include "time.h"
#include "inondation.h"

#define D_DATA 1

typedef struct big_data_t
{
    u_int8_t *content;
    u_int16_t contentlen;
    u_int16_t read_nb;
    struct timespec end_tm;
    u_int8_t type;
} big_data_t;

bool_t init_big_data(void);
void free_big_data(void);
bool_t traitment_data(u_int64_t sender_id, uint8_t type, data_t content);

#endif