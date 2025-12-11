#ifndef FETA_H_
#define FETA_H_

#include "status.h"
#include "jtdev.h"

#define FET_BUFFER_CAPACITY (256 * 1024)

extern unsigned char fet_buffer[FET_BUFFER_CAPACITY];

void command_loop(struct jtdev *p);

#endif
