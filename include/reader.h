#ifndef _READER_H
#define _READER_H

#include <sys/types.h>
#include <sys/socket.h>

#include "writer.h"
#include "iovec.h"
#include "voisin.h"

#define D_READER 1

#define NB_TLV 8
#define READBUF 4096
#define RDHDRLEN 4

ssize_t read_msg();

#endif