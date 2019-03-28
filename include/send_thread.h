#ifndef H_SEND_THREAD
#define H_SEND_THREAD

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "debug.h"
#include "TLV.h"
#include "writer.h"
#include "voisin.h"


#define D_SEND_THREAD 0
#define MIN 8

short init_sender();

#endif // H_SEND_THREAD
