#include "picofet_proto.h"
#include "jtdev.h"
#include "jtaglib.h"

void read_memory(struct jtdev *p, address_t address, address_t length, uint8_t *buffer) {
	address_t cursor = 0;
	uint16_t word;

	p->status = STATUS_OK;
	if (address & 1) {
		word = jtag_read_mem(p, 8, address);
		if (p->status != STATUS_OK) {
			return;
		}
		buffer[cursor] = word;
		cursor += 1;
	}

	while ((length - cursor) >= 2) {
		word = jtag_read_mem(p, 16, address + cursor);
		if (p->status != STATUS_OK) {
			return;
		}
		buffer[cursor+0] = word & 0xff;
		buffer[cursor+1] = (word >> 8) & 0xff;
		cursor += 2;
	}

	if (cursor < length) {
		word = jtag_read_mem(p, 8, address + cursor);
		if (p->status != STATUS_OK) {
			return;
		}
		buffer[cursor] = word;
	}
}

void write_ram(struct jtdev *p, address_t address, address_t length, const uint8_t *buffer) {
	address_t cursor = 0;
	uint16_t word;

	p->status = STATUS_OK;
	if (address & 1) {
		word = buffer[cursor];
		jtag_write_mem(p, 8, address, word);
		if (p->status != STATUS_OK) {
			return;
		}
		cursor += 1;
	}

	while ((length - cursor) >= 2) {
		word = buffer[cursor+0] | (buffer[cursor+1] << 8);
		jtag_write_mem(p, 16, address + cursor, word);
		if (p->status != STATUS_OK) {
			return;
		}
		cursor += 2;
	}

	if (cursor < length) {
		word = buffer[cursor];
		jtag_write_mem(p, 8, address + cursor, word);
		if (p->status != STATUS_OK) {
			return;
		}
	}
}

void write_flash(struct jtdev *p, address_t address, address_t length, const uint8_t *buffer) {
	address_t cursor = 0;
	uint16_t word;

	p->status = STATUS_OK;
	if (address & 1) {
		word = jtag_read_mem(p, 16, (address + cursor) & ~1u);
		word = (word & 0x00ff) | (buffer[cursor] << 8);
		jtag_write_flash_le(p, (address + cursor) & ~1u, 1, buffer + cursor);
		if (p->status != STATUS_OK) {
			return;
		}
		cursor += 1;
	}

	jtag_write_flash_le(p, address + cursor, (length - cursor) / 2, buffer + cursor);
	if (p->status != STATUS_OK) {
		return;
	}
	cursor += (length - cursor) & ~1u;

	if (cursor < length) {
		word = jtag_read_mem(p, 16, (address + cursor) & ~1u);
		word = (word & 0xff00) | buffer[cursor];
		jtag_write_flash_le(p, address + cursor, 1, buffer + cursor);
		if (p->status != STATUS_OK) {
			return;
		}
	}
}

