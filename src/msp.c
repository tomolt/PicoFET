#include <stdbool.h>

#include "link.h"
#include "util.h"

#define IR_ADDR_16BIT		0x83
#define IR_ADDR_CAPTURE		0x84
#define IR_DATA_TO_ADDR		0x85
#define IR_DATA_16BIT		0x41
#define IR_DATA_QUICK		0x43
#define IR_BYPASS		0xFF
#define IR_CNTRL_SIG_16BIT      0x13
#define IR_CNTRL_SIG_CAPTURE	0x14
#define IR_CNTRL_SIG_RELEASE	0x15
#define IR_DATA_PSA		0x44
#define IR_SHIFT_OUT_PSA	0x46
#define IR_CONFIG_FUSES         0x29 // TODO is this correct?
#define IR_EMEX_DATA_EXCHANGE	0x
#define IR_EMEX_WRITE_CONTROL	0x
#define IR_EMEX_READ_CONTROL	0x

/* JTAG Control Signal Register bits */
#define CSR_READ       0x0001
#define CSR_HALT_JTAG  0x0008 // Similar, but not the same semantics between families
#define CSR_BYTE       0x0010
#define CSR_INSTR_LOAD 0x0080 // Similar, but not the same semantics between families
#define CSR_TCE0       0x0200
#define CSR_TCE1       0x0400
#define CSR_POR        0x0800

/* Take device under JTAG control.
 * Run after initial fuse check and reset.
 */
void msp_acquire(struct link *l)
{
	link_ir_shift(l, IR_CNTRL_SIG_16BIT);
	link_dr_shift_16(l, 0x2401);
	link_ir_shift(l, IR_CNTRL_SIG_CAPTURE);
	for (int i = 0; i < 50; i++) {
		uint16_t csr = link_dr_shift_16(l, 0x0000);
		if (csr & CSR_TCE0) {
			return;
		}
	}
	// TODO fail
}

void msp_set_instr_fetch(struct link *l)
{
	link_ir_shift(l, IR_CNTRL_SIG_CAPTURE);
	for (int i = 0; i < 50; i++) {
		uint16_t csr = link_dr_shift_16(l, 0x0000);
		if (csr & CSR_INSTR_LOAD) {
			return;
		}
		// The TCLK pulse before dr_shift_16 leads to
		// problems at MEM_QUICK_READ, it's from SLAU265
		link_tclk_clear(l);
		link_tclk_set(l);
	}
	// TODO fail
}

/* Set the program counter register.
 * MCU must be in the instruction fetch state.
 */
void msp_set_pc(struct link *l, address_t address)
{
	link_ir_shift(l, IR_CNTRL_SIG_16BIT);
	link_dr_shift_16(l, 0x3401); // Release low byte
	link_ir_shift(l, IR_DATA_16BIT);
	link_dr_shift_16(l, 0x4030); // Load PC
	link_tclk_clear(l);
	link_tclk_set(l);
	link_dr_shift_16(l, address);
	link_tclk_clear(l);
	link_tclk_set(l);
	link_ir_shift(l, IR_ADDR_CAPTURE); // Disable IR_DATA_16BIT
	link_tclk_clear(l); // Now PC is set
	link_ir_shift(l, IR_CNTRL_SIG_16BIT);
	link_dr_shift_16(l, 0x2401); // Low byte controlled by JTAG
}

bool msp_is_fuse_blown(struct link *l)
{
	// First attempt could be wrong
	for (int i = 0; i < 3; i++) {
		link_ir_shift(l, IR_CNTRL_SIG_CAPTURE);
		if (link_dr_shift_16(l, 0xAAAA) == 0x5555) {
			return true; // Fuse is blown
		}
	}
	return false; // Fuse is not blown
}

uint8_t msp_get_config_fuses(struct link *l)
{
	link_ir_shift(l, IR_CONFIG_FUSES);
	return link_dr_shift_8(l, 0);
}

void msp_puc(struct link *l)
{
	link_ir_shift(l, IR_CNTRL_SIG_16BIT);
	// TODO
}

/*
void msp_halt(struct link *l)
{
}
*/

uint16_t msp_read_16(struct link *l, address_t address)
{
	msp_halt(l);
	link_tclk_clear(l);
	
	link_ir_shift(l, IR_CNTRL_SIG_16BIT);
	link_dr_shift_16(l, 0x2409);

	link_ir_shift(l, IR_ADDR_16BIT);
	link_dr_shift_16(l, address);
	link_ir_shift(l, IR_DATA_TO_ADDR);
	link_tclk_set(l);
	link_tclk_clear(l);

	uint16_t data = link_dr_shift_16(l, 0x0000);
	link_tclk_set(l); // maybe
	msp_release(l);
	
	return data;
}
