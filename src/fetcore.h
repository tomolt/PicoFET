#ifndef FETA_H_
#define FETA_H_

#include "picofet_proto.h"
#include "jtdev.h"
#include "comm.h"

#define FET_BUFFER_CAPACITY (64 * 1024)

extern unsigned char fet_buffer[FET_BUFFER_CAPACITY];

void send_status(struct comm *t, int status);

#endif
