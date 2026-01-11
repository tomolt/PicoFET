#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "picofet_proto.h"
#include "jtaglib.h"
#include "jtdev.h"
#include "fetcore.h"

#define MAX_ARGS 8

#define TYPECODE_SYMBOL ((int)'y')
#define ARG_SYMBOL      "y"

#define TYPECODE_UINT   ((int)'u')
#define ARG_UINT        "u"

#define TYPECODE_SINT   ((int)'i')
#define ARG_SINT        "i"

union arg_value {
	char         *symbol;
	unsigned long uint;
	long          sint;
};

struct cmd_def {
	const char *name;
	const char *args[MAX_ARGS];
	void      (*func)(struct jtdev *p, struct comm *t, union arg_value *args);
};

void send_status(struct comm *t, int status) {
	char msg[6];
	sprintf(msg, "%03d\r\n", status);
	t->f->write(t, msg, 5);
}

void send_address(struct comm *t, address_t address) {
	char msg[13];
	sprintf(msg, "0x%08" PRIXADDR "\r\n", address);
	t->f->write(t, msg, 12);
}

void cmd_mcu_attach(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;
	unsigned id = jtag_init(p);
	send_status(t, p->status);
	if (p->status == STATUS_OK) {
		send_address(t, id);
	}
}

void cmd_mcu_detach(struct jtdev *p, struct comm *t, union arg_value *args) {
	address_t pc = args[0].uint;
	jtag_release_device(p, pc);
	send_status(t, p->status);
}

void cmd_mcu_get_id(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;
	unsigned id = jtag_chip_id(p);
	send_status(t, p->status);
	if (p->status == STATUS_OK) {
		send_address(t, id);
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
	p->f->jtdev_led_green(p, 1);

	unsigned received = 0;
	while (received < nbytes) {
		received += t->f->read_nb(t, fet_buffer + offset + received, nbytes - received);
	}

	send_status(t, STATUS_OK);
	p->f->jtdev_led_green(p, 0);
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
	p->f->jtdev_led_green(p, 1);

	t->f->write(t, fet_buffer + offset, nbytes);

	send_status(t, STATUS_OK);
	p->f->jtdev_led_green(p, 0);
}

void cmd_ram_read(struct jtdev *p, struct comm *t, union arg_value *args) {
	unsigned long offset  = args[0].uint;
	unsigned long address = args[1].uint;
	unsigned long nbytes  = args[2].uint;
	if (offset >= FET_BUFFER_CAPACITY || nbytes > FET_BUFFER_CAPACITY - offset) {
		send_status(t, STATUS_OUT_OF_BOUNDS);
		return;
	}

	jtag_read_mem_quick(p, address, nbytes / 2, (uint16_t *)(fet_buffer + offset));
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

	jtag_write_mem_quick(p, address, nbytes / 2, (uint16_t *)(fet_buffer + offset));
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

	jtag_write_flash(p, address, nbytes / 2, (uint16_t *)(fet_buffer + offset));
	send_status(t, p->status);
}

void cmd_flash_erase(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)args;
	jtag_erase_flash(p, JTAG_ERASE_MAIN, 0x0);
	send_status(t, p->status);
}

void cmd_reg_read(struct jtdev *p, struct comm *t, union arg_value *args) {
	address_t value = jtag_read_reg(p, args[0].uint);
	send_status(t, p->status);
	send_address(t, value);
}

void cmd_reg_write(struct jtdev *p, struct comm *t, union arg_value *args) {
	jtag_write_reg(p, args[0].uint, args[1].uint);
	send_status(t, p->status);
}

void cmd_info_commands(struct jtdev *p, struct comm *t, union arg_value *args);

const struct cmd_def cmd_defs[] = {
	{
		"INFO:COMMANDS",
		{ NULL },
		cmd_info_commands,
	},
	{
		"MCU:ATTACH",
		{ NULL },
		cmd_mcu_attach,
	},
	{
		"MCU:DETACH",
		{ ARG_UINT "mcu_id" },
		cmd_mcu_detach,
	},
	{
		"MCU:GET_ID",
		{ NULL },
		cmd_mcu_get_id,
	},
	{
		"BUF:CAPACITY",
		{ NULL },
		cmd_buf_capacity,
	},
	{
		"BUF:UPLOAD_BIN",
		{ ARG_UINT "buf_offset", ARG_UINT "num_bytes" },
		cmd_buf_upload_bin,
	},
	{
		"BUF:DOWNLOAD_BIN",
		{ ARG_UINT "buf_offset", ARG_UINT "num_bytes" },
		cmd_buf_download_bin,
	},
	{
		"RAM:READ",
		{ ARG_UINT "buf_offset", ARG_UINT "address", ARG_UINT "num_bytes" },
		cmd_ram_read,
	},
	{
		"RAM:WRITE",
		{ ARG_UINT "buf_offset", ARG_UINT "address", ARG_UINT "num_bytes" },
		cmd_ram_write,
	},
	{
		"RAM:VERIFY",
		{ ARG_UINT "buf_offset", ARG_UINT "address", ARG_UINT "num_bytes" },
		cmd_ram_verify,
	},
	/*{
		"RAM:VERIFY_ERASED",
		ARG_UINT ARG_UINT,
		cmd_ram_verify_erased,
	},*/
	{
		"FLASH:WRITE",
		{ ARG_UINT "buf_offset", ARG_UINT "address", ARG_UINT "num_bytes" },
		cmd_flash_write,
	},
	{
		"FLASH:ERASE",
		{ NULL },
		cmd_flash_erase,
	},
	{
		"REG:READ",
		{ ARG_UINT "reg_idx" },
		cmd_reg_read,
	},
	{
		"REG:WRITE",
		{ ARG_UINT "reg_idx", ARG_SINT "value" },
		cmd_reg_write,
	},
};

void cmd_info_commands(struct jtdev *p, struct comm *t, union arg_value *args) {
	(void)p;
	(void)args;
	send_status(t, STATUS_OK);
	for (unsigned i = 0; i < ARRAY_LEN(cmd_defs); i++) {
		t->f->write(t, cmd_defs[i].name, strlen(cmd_defs[i].name));
		for (int a = 0; cmd_defs[i].args[a]; a++) {
			char buf[32];
			snprintf(buf, sizeof buf, " %s", cmd_defs[i].args[a]+1);
			t->f->write(t, buf, strlen(buf));
		}
		t->f->write(t, "\r\n", 2);
	}
	t->f->write(t, ".\r\n", 3);
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

	cmd->func(p, t, args);
}
