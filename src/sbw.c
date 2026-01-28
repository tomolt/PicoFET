#include <stdint.h>
#include <stdbool.h>

#include "link.h"

/* Note from slau320aj:
 * The low phase of the clock signal supplied on SBWTCK must not be longer than 7 μs.
 * If the low phase is longer, the SBW logic is deactivated,
 * and it must be activated again according to Section 2.3.1.
 */

extern const struct link_func link_func_sbw;

struct link_sbw {
	struct link link;
	bool tclk_latched;
};

static void sbw_delay(void)
{
	// Delay for ~0.27us @ 150MHz MCU frequency
	__asm__ volatile (
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		"nop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
		: );
}

static void sbw_tdio_set(bool value)
{
	(void)value;
}

static bool sbw_tdio_get(void)
{
	return false;
}

static void sbw_tdio_nodrive(void)
{
}

static void sbw_tdio_drive(void)
{
}

static void sbw_tck_low(void)
{
}

static void sbw_tck_high(void)
{
}

static void sbw_tms_restore_tdi(bool tms, bool tdi_init)
{
	sbw_tdio_set(tms);
	sbw_delay();
	sbw_tck_low();
	sbw_delay();
	sbw_tdio_set(tdi_init);
	sbw_tck_high();
}

static void sbw_tdi(bool value)
{
	sbw_tdio_set(value);
	sbw_delay();
	sbw_tck_low();
	sbw_delay();
	sbw_tck_high();
}

static bool sbw_tdo(void)
{
	sbw_tdio_nodrive();
	sbw_delay();
	sbw_tck_low();
	sbw_delay();
	bool value = sbw_tdio_get();
	sbw_delay();
	sbw_tck_high();
	sbw_tdio_drive();
	return value;
}

static void sbw_tdo_noout(void)
{
	sbw_tdio_nodrive();
	sbw_delay();
	sbw_tck_low();
	sbw_delay();
	sbw_tck_high();
	sbw_tdio_drive();
}

static bool sbw_cycle(bool tms, bool tdi_init, bool tdi)
{
	sbw_tms_restore_tdi(tms, tdi_init);
	sbw_tdi(tdi);
	bool tdo = sbw_tdo();
	return tdo;
}

static void sbw_cycle_noout(bool tms, bool tdi_init, bool tdi)
{
	sbw_tms_restore_tdi(tms, tdi_init);
	sbw_tdi(tdi);
	sbw_tdo_noout();
}

static void tap_state_advance(bool tms)
{
	sbw_cycle_noout(tms, tms, false);
}

static uint32_t sbw_shift(unsigned nbits, uint32_t in)
{
	uint32_t out = 0;
	for (unsigned i = 0; i < nbits; i++) {
		bool inb = in & 1;
		in  >>= 1;
		bool outb = sbw_cycle(false, false, inb);
		out <<= 1;
		out |= outb;
	}
	return out;
}

uint8_t sbw_ir_shift(struct link *l, uint8_t in)
{
	struct link_sbw *sbw = (void *)l;
	// Run-Test/Idle ~> Shift-IR
	tap_state_advance(true);
	tap_state_advance(true);
	tap_state_advance(false);
	tap_state_advance(false);
	uint8_t out = sbw_shift(8, in);
	// Back to Run-Test/Idle; Preserve TCLK
	tap_state_advance(true);
	tap_state_advance(true);
	sbw_cycle_noout(false, sbw->tclk_latched, sbw->tclk_latched);
	return out;
}

uint8_t sbw_dr_shift_8(struct link *l, uint8_t in)
{
	struct link_sbw *sbw = (void *)l;
	// Run-Test/Idle ~> Shift-DR
	tap_state_advance(true);
	tap_state_advance(false);
	tap_state_advance(false);
	uint8_t out = sbw_shift(8, in);
	// Back to Run-Test/Idle; Preserve TCLK
	tap_state_advance(true);
	tap_state_advance(true);
	sbw_cycle_noout(false, sbw->tclk_latched, sbw->tclk_latched);
	return out;
}

uint16_t sbw_dr_shift_16(struct link *l, uint16_t in)
{
	struct link_sbw *sbw = (void *)l;
	// Run-Test/Idle ~> Shift-DR
	tap_state_advance(true);
	tap_state_advance(false);
	tap_state_advance(false);
	uint16_t out = sbw_shift(16, in);
	// Back to Run-Test/Idle; Preserve TCLK
	tap_state_advance(true);
	tap_state_advance(true);
	sbw_cycle_noout(false, sbw->tclk_latched, sbw->tclk_latched);
	return out;
}

void sbw_tclk_clear(struct link *l)
{
	struct link_sbw *sbw = (void *)l;
	sbw_cycle_noout(false, sbw->tclk_latched, false);
	sbw->tclk_latched = false;
}

void sbw_tclk_set(struct link *l)
{
	struct link_sbw *sbw = (void *)l;
	sbw_cycle_noout(false, sbw->tclk_latched, true);
	sbw->tclk_latched = true;
}

const struct link_func link_func_sbw = {
	.ir_shift    = sbw_ir_shift,
	.dr_shift_8  = sbw_dr_shift_8,
	.dr_shift_16 = sbw_dr_shift_16,
	.tclk_clear  = sbw_tclk_clear,
	.tclk_set    = sbw_tclk_set,
};
