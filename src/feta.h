#ifndef FETA_H_
#define FETA_H_

#include "status.h"
#include "jtdev.h"
#include "transport.h"

void feta_init(struct transport *t, struct jtdev *p);
void process_command(char *line);

#endif
