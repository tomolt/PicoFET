#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "picofet_proto.h"
#include "jtaglib.h"
#include "jtdev.h"
#include "comm.h"
#include "cmd.h"
#include "ops.h"
#include "version.h"

#define MAX_COMMAND_LENGTH 256
#define MAX_ARGS 8

#define TYPECODE_SYMBOL ((int)'y')
#define ARG_SYMBOL      "y"

#define TYPECODE_UINT   ((int)'u')
#define ARG_UINT        "u"

#define TYPECODE_SINT   ((int)'i')
#define ARG_SINT        "i"

#define ATTACH_NOT_NEEDED 0x1

union arg_value {
	char         *symbol;
	unsigned long uint;
	long          sint;
};

struct cmd_def {
	const char *name;
	const char *args[MAX_ARGS];
	void      (*func)(struct jtdev *p, struct comm *t, union arg_value *args);
	int         flags;
};

unsigned char fet_buffer[FET_BUFFER_CAPACITY];

char command_line[MAX_COMMAND_LENGTH];

void send_status(struct comm *t, int status) {
	char msg[64];
	int length = snprintf(msg, sizeof msg, "%03d %s\r\n", status, pfet_get_status_message(status));
	t->f->comm_write(t, msg, length);
}

void send_address(struct comm *t, address_t address) {
	char msg[13];
	snprintf(msg, sizeof msg, "0x%08" PRIXADDR "\r\n", address);
	t->f->comm_write(t, msg, 12);
}

void cmd_mcu_attach(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;

	p->status = STATUS_OK;
	unsigned id = jtag_init(p);

	send_status(t, p->status);
	if (p->status == STATUS_OK) {
		send_address(t, id);
	}
}

void cmd_mcu_detach(struct jtdev *p, struct comm *t, union arg_value *args) {
	address_t pc = args[0].uint;
	
	p->status = STATUS_OK;
	jtag_release_device(p, pc);
	
	send_status(t, p->status);
}

void cmd_mcu_get_id(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;

	p->status = STATUS_OK;
	unsigned id = jtag_chip_id(p);
	
	send_status(t, p->status);
	if (p->status == STATUS_OK) {
		send_address(t, id);
	}
}

void cmd_mcu_reset(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;

	p->status = STATUS_OK;
	jtag_execute_puc(p); // FIXME
	send_status(t, p->status);
}

void cmd_mcu_continue(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;

	p->status = STATUS_OK;
	jtag_release_device(p, 0xffff);
	send_status(t, p->status);
}

void cmd_mcu_halt(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;

	p->status = STATUS_OK;
	jtag_get_device(p);
	send_status(t, p->status);
}

void cmd_mcu_step(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;

	p->status = STATUS_OK;
	jtag_single_step(p);
	send_status(t, p->status);
}

void cmd_mcu_is_halted(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;

	p->status = STATUS_OK;
	int halted = jtag_cpu_state(p);
	send_status(t, p->status);
	if (p->status == STATUS_OK) {
		send_address(t, halted);
	}
}

void cmd_buf_capacity(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)p;
	(void)args;

	send_status(t, STATUS_OK);
	send_address(t, FET_BUFFER_CAPACITY);
}

void cmd_buf_upload_bin(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)p;
	
	unsigned long offset = args[0].uint;
	unsigned long nbytes = args[1].uint;
	if (offset >= FET_BUFFER_CAPACITY || nbytes > FET_BUFFER_CAPACITY - offset) {
		send_status(t, STATUS_OUT_OF_BOUNDS);
		return;
	}

	send_status(t, STATUS_CONTINUE_TRANSFER);

	unsigned received = 0;
	while (received < nbytes) {
		received += t->f->comm_read_nb(t, fet_buffer + offset + received, nbytes - received);
	}

	send_status(t, STATUS_OK);
}

void cmd_buf_download_bin(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)p;
	
	unsigned long offset = args[0].uint;
	unsigned long nbytes = args[1].uint;
	if (offset >= FET_BUFFER_CAPACITY || nbytes > FET_BUFFER_CAPACITY - offset) {
		send_status(t, STATUS_OUT_OF_BOUNDS);
		return;
	}

	send_status(t, STATUS_CONTINUE_TRANSFER);

	t->f->comm_write(t, fet_buffer + offset, nbytes);

	send_status(t, STATUS_OK);
}

void cmd_ram_read(struct jtdev *p, struct comm *t, union arg_value *args) {
	unsigned long offset  = args[0].uint;
	unsigned long address = args[1].uint;
	unsigned long nbytes  = args[2].uint;
	if (offset >= FET_BUFFER_CAPACITY || nbytes > FET_BUFFER_CAPACITY - offset) {
		send_status(t, STATUS_OUT_OF_BOUNDS);
		return;
	}

	p->status = STATUS_OK;
	read_memory(p, address, nbytes, fet_buffer + offset);
	send_status(t, p->status);
}

void cmd_ram_write(struct jtdev *p, struct comm *t, union arg_value *args) {
	unsigned long offset  = args[0].uint;
	unsigned long address = args[1].uint;
	unsigned long nbytes  = args[2].uint;
	if (offset >= FET_BUFFER_CAPACITY || nbytes > FET_BUFFER_CAPACITY - offset) {
		send_status(t, STATUS_OUT_OF_BOUNDS);
		return;
	}

	p->status = STATUS_OK;
	write_ram(p, address, nbytes, fet_buffer + offset);
	send_status(t, p->status);
}

void cmd_ram_verify(struct jtdev *p, struct comm *t, union arg_value *args) {
	unsigned long offset  = args[0].uint;
	unsigned long address = args[1].uint;
	unsigned long nbytes  = args[2].uint;
	if (offset >= FET_BUFFER_CAPACITY || nbytes > FET_BUFFER_CAPACITY - offset) {
		send_status(t, STATUS_OUT_OF_BOUNDS);
		return;
	}

	p->status = STATUS_OK;
	// FIXME implement misaligned verify
	int ok = jtag_verify_mem(p, address, nbytes / 2, (uint16_t *)(fet_buffer + offset));
	send_status(t, ok ? p->status : STATUS_CONTENT_MISMATCH);
}

void cmd_flash_write(struct jtdev *p, struct comm *t, union arg_value *args) {
	unsigned long offset  = args[0].uint;
	unsigned long address = args[1].uint;
	unsigned long nbytes  = args[2].uint;
	if (offset >= FET_BUFFER_CAPACITY || nbytes > FET_BUFFER_CAPACITY - offset) {
		send_status(t, STATUS_OUT_OF_BOUNDS);
		return;
	}

	p->status = STATUS_OK;
	write_flash(p, address, nbytes, fet_buffer + offset);
	send_status(t, p->status);
}

void cmd_flash_erase_all(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;

	p->status = STATUS_OK;
	jtag_erase_flash(p, JTAG_ERASE_MASS, 0x0);

	send_status(t, p->status);
}

void cmd_flash_erase_main(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;

	p->status = STATUS_OK;
	jtag_erase_flash(p, JTAG_ERASE_MAIN, 0x0);

	send_status(t, p->status);
}

void cmd_flash_erase_seg(struct jtdev *p, struct comm *t, union arg_value *args) {
	address_t address = args[0].uint;

	p->status = STATUS_OK;
	jtag_erase_flash(p, JTAG_ERASE_SGMT, address);

	send_status(t, p->status);
}

void cmd_reg_read(struct jtdev *p, struct comm *t, union arg_value *args) {
	p->status = STATUS_OK;
	address_t value = jtag_read_reg(p, args[0].uint);

	send_status(t, p->status);
	if (p->status == STATUS_OK) {
		send_address(t, value);
	}
}

void cmd_reg_write(struct jtdev *p, struct comm *t, union arg_value *args) {
	p->status = STATUS_OK;
	jtag_write_reg(p, args[0].uint, args[1].uint);

	send_status(t, p->status);
}

void cmd_fuses_get_config(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;

	p->status = STATUS_OK;
	int fuses = jtag_get_config_fuses(p);

	send_status(t, p->status);
	if (p->status == STATUS_OK) {
		send_address(t, fuses);
	}
}

void cmd_break_clear_all(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;

	p->status = STATUS_OK;
	jtag_set_breakpoint(p, -1, 0x0);

	send_status(t, p->status);
}

void cmd_break_set(struct jtdev *p, struct comm *t, union arg_value *args) {
	unsigned long bp_idx  = args[0].uint;
	unsigned long address = args[1].uint;

	p->status = STATUS_OK;
	int ret = jtag_set_breakpoint(p, bp_idx, address);

	if (ret == 1) {
		send_status(t, p->status);
	} else {
		send_status(t, STATUS_TOO_MANY_BREAKS);
	}
}

void cmd_version(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)p;
	(void)args;

	address_t version = (PICOFET_MAJOR_VERSION << 16) | (PICOFET_MINOR_VERSION << 8);

	send_status(t, STATUS_OK);
	send_address(t, version);
}

void cmd_help(struct jtdev *p, struct comm *t, union arg_value *args);

const struct cmd_def cmd_defs[] = {
	{
		"HELP",
		{ NULL },
		cmd_help,
		ATTACH_NOT_NEEDED
	},
	{
		"VERSION",
		{ NULL },
		cmd_version,
		ATTACH_NOT_NEEDED
	},
	{
		"MCU:ATTACH",
		{ NULL },
		cmd_mcu_attach,
		ATTACH_NOT_NEEDED
	},
	{
		"MCU:DETACH",
		{ ARG_UINT "mcu_id" },
		cmd_mcu_detach,
		0
	},
	{
		"MCU:GET_ID",
		{ NULL },
		cmd_mcu_get_id,
		0
	},
	{
		"MCU:RESET",
		{ NULL },
		cmd_mcu_reset,
		0
	},
	{
		"MCU:CONTINUE",
		{ NULL },
		cmd_mcu_continue,
		0
	},
	{
		"MCU:HALT",
		{ NULL },
		cmd_mcu_halt,
		0
	},
	{
		"MCU:STEP",
		{ NULL },
		cmd_mcu_step,
		0
	},
	{
		"BUF:CAPACITY",
		{ NULL },
		cmd_buf_capacity,
		ATTACH_NOT_NEEDED
	},
	{
		"BUF:UPLOAD_BIN",
		{ ARG_UINT "buf_offset", ARG_UINT "num_bytes", NULL },
		cmd_buf_upload_bin,
		ATTACH_NOT_NEEDED
	},
	{
		"BUF:DOWNLOAD_BIN",
		{ ARG_UINT "buf_offset", ARG_UINT "num_bytes", NULL },
		cmd_buf_download_bin,
		ATTACH_NOT_NEEDED
	},
	{
		"RAM:READ",
		{ ARG_UINT "buf_offset", ARG_UINT "address", ARG_UINT "num_bytes", NULL },
		cmd_ram_read,
		0
	},
	{
		"RAM:WRITE",
		{ ARG_UINT "buf_offset", ARG_UINT "address", ARG_UINT "num_bytes", NULL },
		cmd_ram_write,
		0
	},
	{
		"RAM:VERIFY",
		{ ARG_UINT "buf_offset", ARG_UINT "address", ARG_UINT "num_bytes", NULL },
		cmd_ram_verify,
		0
	},
	{
		"FLASH:WRITE",
		{ ARG_UINT "buf_offset", ARG_UINT "address", ARG_UINT "num_bytes", NULL },
		cmd_flash_write,
		0
	},
	{
		"FLASH:ERASE_ALL",
		{ NULL },
		cmd_flash_erase_all,
		0
	},
	{
		"FLASH:ERASE_MAIN",
		{ NULL },
		cmd_flash_erase_main,
		0
	},
	{
		"FLASH:ERASE_SEG",
		{ ARG_UINT "address", NULL },
		cmd_flash_erase_seg,
		0
	},
	{
		"REG:READ",
		{ ARG_UINT "reg_idx", NULL },
		cmd_reg_read,
		0
	},
	{
		"REG:WRITE",
		{ ARG_UINT "reg_idx", ARG_SINT "value", NULL },
		cmd_reg_write,
		0
	},
	{
		"FUSES:GET_CONFIG",
		{ NULL },
		cmd_fuses_get_config,
		0
	},
	{
		"BREAK:CLEAR_ALL",
		{ NULL },
		cmd_break_clear_all,
		0
	},
	{
		"BREAK:SET",
		{ ARG_UINT "bp_idx", ARG_UINT "address", NULL },
		cmd_break_set,
		0
	},
};

void cmd_help(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)p;
	(void)args;
	send_status(t, STATUS_OK);
	for (unsigned i = 0; i < ARRAY_LEN(cmd_defs); i++) {
		t->f->comm_write(t, cmd_defs[i].name, strlen(cmd_defs[i].name));
		for (int a = 0; cmd_defs[i].args[a]; a++) {
			char buf[32];
			snprintf(buf, sizeof buf, " %s", cmd_defs[i].args[a]+1);
			t->f->comm_write(t, buf, strlen(buf));
		}
		t->f->comm_write(t, "\r\n", 2);
	}
	t->f->comm_write(t, ".\r\n", 3);
}

void process_command(struct jtdev *p, struct comm *t, char *line) {
	const char *tok_separators = " \t\r\n";
	
	char *tok, *saveptr, *end;

	// Isolate the command name and find it in the command list
	if (!(tok = strtok_r(line, tok_separators, &saveptr))) {
		return;
	}
	const struct cmd_def *cmd = NULL;
	for (unsigned i = 0; i < ARRAY_LEN(cmd_defs); i++) {
		if (strcasecmp(cmd_defs[i].name, tok) == 0) {
			cmd = &cmd_defs[i];
			break;
		}
	}
	if (!cmd) {
		send_status(t, STATUS_UNKNOWN_COMMAND);
		return;
	}

	// Parse command arguments
	union arg_value args[MAX_ARGS];
	unsigned argc = 0;
	while ((tok = strtok_r(NULL, tok_separators, &saveptr))) {
		switch (cmd->args[argc][0]) {
		case TYPECODE_SYMBOL:
			args[argc].symbol = tok;
			break;

		case TYPECODE_UINT:
			errno = 0;
			args[argc].uint = strtoul(tok, &end, 0);
			if (*end != '\0') {
				send_status(t, STATUS_INVALID_ARGUMENTS);
				return;
			}
			if (errno) {
				send_status(t, STATUS_INTEGER_OVERFLOW);
				return;
			}
			break;

		case TYPECODE_SINT:
			errno = 0;
			args[argc].sint = strtol(tok, &end, 0);
			if (*end != '\0') {
				send_status(t, STATUS_INVALID_ARGUMENTS);
				return;
			}
			if (errno) {
				send_status(t, STATUS_INTEGER_OVERFLOW);
				return;
			}
			break;

		default:
			send_status(t, STATUS_INVALID_ARGUMENTS);
			return;
		}

		argc++;
	}
	if (cmd->args[argc]) {
		send_status(t, STATUS_INVALID_ARGUMENTS);
		return;
	}

	if (!(cmd->flags & ATTACH_NOT_NEEDED)) {
		if (!p->attached) {
			send_status(t, STATUS_NOT_ATTACHED);
			return;
		}
	}

	cmd->func(p, t, args);
}

void command_loop(struct jtdev *p, struct comm *t) {
	size_t buffered = 0;
	for (;;) {
		char *lf, c;

		buffered += t->f->comm_read_nb(t, command_line + buffered, sizeof command_line - buffered);

		while ((lf = memchr(command_line, '\n', buffered))) {
			*lf = '\0';
			process_command(p, t, command_line);
			t->f->comm_flush_out(t);
			buffered -= lf + 1 - command_line;
			memmove(command_line, lf + 1, buffered);
		}

		if (buffered == sizeof command_line) {
			// discard any further input until we have reached a new line
			do {
				t->f->comm_read_nb(t, &c, 1);
			} while (c != '\n');
			buffered = 0;
			send_status(t, STATUS_COMMAND_TOO_LONG);
		}
	}
}
