#ifndef FETA_H_
#define FETA_H_

#include "status.h"
#include "jtdev.h"
#include "transport.h"

#define FET_BUFFER_CAPACITY (64 * 1024)

extern unsigned char fet_buffer[FET_BUFFER_CAPACITY];

void process_command(struct jtdev *p, struct transport *t, char *line);
void send_status(struct transport *t, int status);

#endif
