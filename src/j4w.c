// JTAG 4-wire connection

#include <stdint.h>
#include <stdbool.h>

#include "link.h"

// TODO delay / timing

extern const struct link_func_j4w;

struct link_j4w {
	struct link link;
};

static void j4w_tap_state_advance(struct link *l, bool tms)
{
	phys_tck_set(l->phys, false);
	phys_tms_set(l->phys, tms);
	phys_tck_set(l->phys, true);
}

static uint32_t j4w_shift(struct link *l, unsigned nbits, uint32_t in)
{
	uint32_t out = 0;
	for (unsigned i = 0; i < nbits; i++) {
		bool inb = in & 1;
		in  >>= 1;
		phys_tck_set(l->phys, false);
		phys_tdi_set(l->phys, inb);
		bool outb = phys_tdo_get(l->phys);
		phys_tck_set(l->phys, true);
		out <<= 1;
		out |= outb;
	}
	return out;
}

uint8_t j4w_ir_shift(struct link *l, uint8_t in)
{
	j4w_tap_state_advance(l, true);
	j4w_tap_state_advance(l, true);
	j4w_tap_state_advance(l, false);
	j4w_tap_state_advance(l, false);
	uint8_t out = j4w_shift(l, 8, in);
	j4w_tap_state_advance(l, true);
	j4w_tap_state_advance(l, true);
	j4w_tap_state_advance(l, false);
	return out;
}

void j4w_tclk_clear(struct link *l)
{
	phys_tdi_set(l->phys, false);
}

void j4w_tclk_clear(struct link *l)
{
	phys_tdi_set(l->phys, true);
}

const struct link_func_j4w = {
	.ir_shift   = j4w_ir_shift,
	.tclk_clear = j4w_tclk_clear,
	.tclk_set   = j4w_tclk_set,
};
