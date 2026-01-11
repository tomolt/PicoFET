#ifndef PICOFET_OPS_H_
#define PICOFET_OPS_H_

#include "util.h"

struct jtdev; // declared somewhere else

void read_memory(struct jtdev *p, address_t address, address_t length, uint8_t *buffer);
void write_ram(struct jtdev *p, address_t address, address_t length, const uint8_t *buffer);
void write_flash(struct jtdev *p, address_t address, address_t length, const uint8_t *buffer);

#endif
