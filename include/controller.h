#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "reader.h"
#include "writer.h"

#define D_CONTROL 1

extern u_int32_t g_socket;

int create_socket(uint16_t port);
void launch_program(void);

#endif