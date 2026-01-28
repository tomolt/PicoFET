#ifndef PICOFET_LINK_H
#define PICOFET_LINK_H

#include <stdint.h>

struct phys;

struct link_func;
struct link {
	const struct link_func *f;
	struct phys *phys;
};

struct link_func {
	uint8_t  (*ir_shift)   (struct link *l, uint8_t  in);
	uint8_t  (*dr_shift_8) (struct link *l, uint8_t  in);
	uint16_t (*dr_shift_16)(struct link *l, uint16_t in);
	void     (*tclk_clear) (struct link *l);
	void     (*tclk_set)   (struct link *l);
};

#define link_ir_shift(l, in)    ((l)->f->ir_shift(l, in))
#define link_dr_shift_8(l, in)  ((l)->f->dr_shift_8(l, in))
#define link_dr_shift_16(l, in) ((l)->f->dr_shift_16(l, in))
#define link_tclk_clear(l)      ((l)->f->tclk_clear(l))
#define link_tclk_set(l)        ((l)->f->tclk_set(l))

#endif
