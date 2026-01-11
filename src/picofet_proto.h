/* MSPDebug - debugging tool for MSP430 MCUs
 * Copyright (C) 2025-2026 Thomas Oltmann
 *
 * PicoFET is an open-source firmware for the Raspberry Pi Pico and compatible
 * MCUs that turns them into MSP430-compatible JTAG debug probes.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef PICOFET_PROTO_H_
#define PICOFET_PROTO_H_

/* Not finalized yet */
#define PFET_DECLARE_STATUS_CODES(X)\
	X(200, OK,                "Okay")\
	X(500, UNKNOWN_COMMAND,   "Unknown Command")\
	X(501, INVALID_ARGUMENTS, "Invalid Command Arguments")\
	X(502, INTEGER_OVERFLOW,  "Integer Overflow")\
	X(503, COMMAND_TOO_LONG,  "Command Too Long")\
	X(550, FUSE_BLOWN,        "JTAG Security Fuse is blown")\
	X(551, INVALID_JTAG_ID,   "Invalid JTAG ID")\
	X(552, PUC_FAILED,        "PUC Failed")\
	X(553, TOO_MANY_BREAKS,   "Too many Breakpoints")\
	X(554, OUT_OF_BOUNDS,     "Address or Size is Out of Bounds")\
	X(201, CONTENT_MISMATCH,  "Verification succeeded, but contents differ")\
	X(350, CONTINUE_TRANSFER, "Go Ahead with Transfer")\
	X(400, TIMED_OUT,         "JTAG connection with MCU timed out")\
	X(401, TRANSFER_FAILED,   "Transfer failed")\
	X(402, PROGRAMMER_FROZE,  "Programmer Froze")\
	/* end of status code list */

enum {
#define PFET_MAKE_ENUM_(code, name, descr) STATUS_##name = code,
	PFET_DECLARE_STATUS_CODES(PFET_MAKE_ENUM_)
#undef PFET_MAKE_ENUM_
};

static inline const char *pfet_get_status_message(int status)
{
	switch (status) {
#define PFET_MAKE_CASE_(code, name, descr) case code: return descr;
	PFET_DECLARE_STATUS_CODES(PFET_MAKE_CASE_)
#undef PFET_MAKE_CASE_
	default: return "(Unrecognized Status Code)";
	}
}

#endif
