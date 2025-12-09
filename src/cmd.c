#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "status.h"
#include "jtaglib.h"
#include "jtdev.h"

#define MAX_ARGS 8

#define TYPECODE_SYMBOL ((int)'y')
#define SIG_SYMBOL      "y"

#define TYPECODE_UINT   ((int)'u')
#define SIG_UINT        "u"

#define TYPECODE_SINT   ((int)'i')
#define SIG_SINT        "i"

struct jtdev jtdev;

union arg_value {
	char         *symbol;
	unsigned long uint;
	long          sint;
};

struct cmd_def {
	const char *name;
	const char *signature;
	void      (*func)(union arg_value *args);
};

void send_status(int status) {
	printf("%3d\n", status);
}

void cmd_upload(union arg_value *args) {
	(void)args;
}

void cmd_download(union arg_value *args) {
	(void)args;
}

void cmd_read_mem(union arg_value *args) {
	jtag_read_mem_quick(&jtdev, args[0].uint, args[1].uint, NULL);
	send_status(STATUS_OK);
}

void cmd_write_mem(union arg_value *args) {
	jtag_write_mem_quick(&jtdev, args[0].uint, args[1].uint, NULL);
	send_status(STATUS_OK);
}

void cmd_verify_mem(union arg_value *args) {
	(void)args;
}

void cmd_read_reg(union arg_value *args) {
	address_t value = jtag_read_reg(&jtdev, args[0].uint);
	send_status(STATUS_OK);
	printf("0x%32" PRIXADDR "\n", value);
}

void cmd_write_reg(union arg_value *args) {
	jtag_write_reg(&jtdev, args[0].uint, args[1].uint);
	send_status(STATUS_OK);
}

const struct cmd_def cmd_defs[] = {
	{
		"upload",
		SIG_SYMBOL,
		cmd_upload,
	},
	{
		"download",
		SIG_SYMBOL,
		cmd_download,
	},
	{
		"read_mem",
		SIG_UINT SIG_UINT,
		cmd_read_mem,
	},
	{
		"write_mem",
		SIG_UINT SIG_UINT,
		cmd_write_mem,
	},
	{
		"verify_mem",
		SIG_SYMBOL SIG_UINT SIG_UINT,
		cmd_verify_mem,
	},
	{
		"read_reg",
		SIG_UINT,
		cmd_read_reg,
	},
	{
		"write_reg",
		SIG_UINT SIG_SINT,
		cmd_write_reg,
	},
};

void process_command(char *line) {
	char *tok, *saveptr, *end;

	// Isolate the command name and find it in the command list
	if (!(tok = strtok_r(line, " \t", &saveptr))) {
		return;
	}
	const struct cmd_def *cmd = NULL;
	for (unsigned i = 0; i < ARRAY_LEN(cmd_defs); i++) {
		if (strcmp(cmd_defs[i].name, tok) == 0) {
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
	while ((tok = strtok_r(NULL, " \t", &saveptr))) {
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

	cmd->func(args);
}
