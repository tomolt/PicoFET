#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "status.h"
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

#define ARG_NONE     ""

struct jtdev jtdev;

union arg_value {
	char         *symbol;
	unsigned long uint;
	long          sint;
};

struct cmd_def {
	const char *name;
	const char *signature;
	void      (*func)(struct jtdev *p, union arg_value *args);
};

void send_status(int status) {
	printf("%3d\n", status);
}

void send_address(address_t address) {
	printf("0x%8" PRIXADDR "\n", address);
}

void cmd_buf_capacity(struct jtdev *p, union arg_value *args) {
	(void)p;
	(void)args;
	send_status(STATUS_OK);
	send_address(FET_BUFFER_CAPACITY);
}

void cmd_buf_upload_bin(struct jtdev *p, union arg_value *args) {
	(void)p;
	
	unsigned long offset = args[0].uint;
	unsigned long nbytes = args[1].uint;
	if (offset >= FET_BUFFER_CAPACITY || nbytes > FET_BUFFER_CAPACITY - offset) {
		send_status(STATUS_OUT_OF_BOUNDS);
		return;
	}

	send_status(STATUS_CONTINUE_TRANSFER);

	size_t r = fread(fet_buffer + offset, 1, nbytes, stdin);
	if (r == nbytes) {
		send_status(STATUS_OK);
	} else {
		send_status(STATUS_TRANSFER_FAILED);
	}
}

void cmd_buf_download_bin(struct jtdev *p, union arg_value *args) {
	(void)p;
	
	unsigned long offset = args[0].uint;
	unsigned long nbytes = args[1].uint;
	if (offset >= FET_BUFFER_CAPACITY || nbytes > FET_BUFFER_CAPACITY - offset) {
		send_status(STATUS_OUT_OF_BOUNDS);
		return;
	}

	send_status(STATUS_CONTINUE_TRANSFER);

	size_t r = fwrite(fet_buffer + offset, 1, nbytes, stdin);
	if (r == nbytes) {
		send_status(STATUS_OK);
	} else {
		send_status(STATUS_TRANSFER_FAILED);
	}
}

void cmd_ram_read(struct jtdev *p, union arg_value *args) {
	unsigned long offset  = args[0].uint;
	unsigned long address = args[1].uint;
	unsigned long nbytes  = args[2].uint;
	if (offset >= FET_BUFFER_CAPACITY || nbytes > FET_BUFFER_CAPACITY - offset) {
		send_status(STATUS_OUT_OF_BOUNDS);
		return;
	}

	jtag_read_mem_quick(&jtdev, address, nbytes / 2, (uint16_t *)(fet_buffer + offset));
	send_status(STATUS_OK);
}

void cmd_ram_write(struct jtdev *p, union arg_value *args) {
	unsigned long offset  = args[0].uint;
	unsigned long address = args[1].uint;
	unsigned long nbytes  = args[2].uint;
	if (offset >= FET_BUFFER_CAPACITY || nbytes > FET_BUFFER_CAPACITY - offset) {
		send_status(STATUS_OUT_OF_BOUNDS);
		return;
	}

	jtag_write_mem_quick(&jtdev, address, nbytes / 2, (uint16_t *)(fet_buffer + offset));
	send_status(STATUS_OK);
}

void cmd_ram_verify(struct jtdev *p, union arg_value *args) {
	unsigned long offset  = args[0].uint;
	unsigned long address = args[1].uint;
	unsigned long nbytes  = args[2].uint;
	if (offset >= FET_BUFFER_CAPACITY || nbytes > FET_BUFFER_CAPACITY - offset) {
		send_status(STATUS_OUT_OF_BOUNDS);
		return;
	}

	int ok = jtag_verify_mem(&jtdev, address, nbytes / 2, (uint16_t *)(fet_buffer + offset));
	send_status(ok ? STATUS_OK : STATUS_CONTENT_MISMATCH);
}

void cmd_reg_read(struct jtdev *p, union arg_value *args) {
	address_t value = jtag_read_reg(&jtdev, args[0].uint);
	send_status(STATUS_OK);
	send_address(value);
}

void cmd_reg_write(struct jtdev *p, union arg_value *args) {
	jtag_write_reg(&jtdev, args[0].uint, args[1].uint);
	send_status(STATUS_OK);
}

void cmd_info_commands(struct jtdev *p, union arg_value *args);

const struct cmd_def cmd_defs[] = {
	{
		"INFO:COMMANDS",
		ARG_NONE,
		cmd_info_commands,
	},
	{
		"BUF:CAPACITY",
		ARG_NONE,
		cmd_buf_capacity,
	},
	{
		"BUF:UPLOAD_BIN",
		ARG_UINT ARG_UINT,
		cmd_buf_upload_bin,
	},
	{
		"BUF:DOWNLOAD_BIN",
		ARG_UINT ARG_UINT,
		cmd_buf_download_bin,
	},
	{
		"RAM:READ",
		ARG_UINT ARG_UINT ARG_UINT,
		cmd_ram_read,
	},
	{
		"RAM:WRITE",
		ARG_UINT ARG_UINT ARG_UINT,
		cmd_ram_write,
	},
	{
		"RAM:VERIFY",
		ARG_UINT ARG_UINT ARG_UINT,
		cmd_ram_verify,
	},
	{
		"REG:READ",
		ARG_UINT,
		cmd_reg_read,
	},
	{
		"REG:WRITE",
		ARG_UINT ARG_SINT,
		cmd_reg_write,
	},
};

void cmd_info_commands(struct jtdev *p, union arg_value *args) {
	(void)p;
	(void)args;
	send_status(STATUS_OK);
	for (unsigned i = 0; i < ARRAY_LEN(cmd_defs); i++) {
		printf("%s\n", cmd_defs[i].name);
	}
	printf(".\n");
}

void process_command(struct jtdev *p, char *line) {
	char *tok, *saveptr, *end;

	// Isolate the command name and find it in the command list
	if (!(tok = strtok_r(line, " \t\n", &saveptr))) {
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
		send_status(STATUS_UNKNOWN_COMMAND);
		return;
	}

	// Parse command arguments
	union arg_value args[MAX_ARGS];
	unsigned argc = 0;
	while ((tok = strtok_r(NULL, " \t\n", &saveptr))) {
		switch (cmd->signature[argc]) {
		case TYPECODE_SYMBOL:
			args[argc].symbol = tok;
			break;

		case TYPECODE_UINT:
			errno = 0;
			args[argc].uint = strtoul(tok, &end, 0);
			if (*end != '\0') {
				send_status(STATUS_INVALID_ARGUMENTS);
				return;
			}
			if (errno) {
				send_status(STATUS_INTEGER_OVERFLOW);
				return;
			}
			break;

		case TYPECODE_SINT:
			errno = 0;
			args[argc].sint = strtol(tok, &end, 0);
			if (*end != '\0') {
				send_status(STATUS_INVALID_ARGUMENTS);
				return;
			}
			if (errno) {
				send_status(STATUS_INTEGER_OVERFLOW);
				return;
			}
			break;

		default:
			send_status(STATUS_INVALID_ARGUMENTS);
			return;
		}

		argc++;
	}
	if (cmd->signature[argc]) {
		send_status(STATUS_INVALID_ARGUMENTS);
		return;
	}

	cmd->func(p, args);
}

void command_loop(struct jtdev *p) {
	static char buf[1024];
	for (;;) {
		fgets(buf, sizeof buf, stdin);
		size_t len = strlen(buf);
		if (len == 0 || buf[len-1] != '\n') {
			// discard any further input until we have reached a new line
			// TODO work around EOF
			while (fgetc(stdin) != '\n') {}
			send_status(STATUS_COMMAND_TOO_LONG);
			continue;
		}

		process_command(p, buf);
	}
}
