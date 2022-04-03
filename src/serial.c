// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "serial.h"
#include "glue.h"

serial_port_t serial_port;

static int state = 0;
static bool valid;
static int bit;
static uint8_t byte;
static bool listening = false;
static bool during_atn = false;
static bool eoi = false;
static int clocks_since_last_change = 0;

void
serial_step(int clocks)
{
	static serial_port_t old_serial_port;
	if (old_serial_port.atn_in == serial_port.atn_in &&
		old_serial_port.clk_in == serial_port.clk_in &&
		old_serial_port.data_in == serial_port.data_in) {
		clocks_since_last_change += clocks;
		if (state == 2 && valid == true && bit == 0 && clocks_since_last_change > 200 * MHZ) {
			if (clocks_since_last_change < (200 + 60) * MHZ) {
				printf("XXX EOI ACK\n");
				serial_port.data_out = 0;
				eoi = true;
			} else {
				printf("XXX EOI ACK END\n");
				serial_port.data_out = 1;
				clocks_since_last_change = 0;
			}
		}
		return;
	}

	clocks_since_last_change = 0;

	printf("-SERIAL IN { ATN:%d CLK:%d DATA:%d } --- OUT { CLK:%d DATA:%d }\n", old_serial_port.atn_in, old_serial_port.clk_in, old_serial_port.data_in, old_serial_port.clk_out, old_serial_port.data_out);
	printf("+SERIAL IN { ATN:%d CLK:%d DATA:%d } --- OUT { CLK:%d DATA:%d } -- #%d\n", serial_port.atn_in, serial_port.clk_in, serial_port.data_in, serial_port.clk_out, serial_port.data_out, state);

	if (!during_atn && serial_port.atn_in) {
		serial_port.data_out = 0;
		state = 1;
		during_atn = true;
		printf("XXX START OF ATN\n");
	}

	switch(state) {
		case 0:
			break;
		case 1:
			if (during_atn && !serial_port.atn_in) {
				// cancelled ATN
				serial_port.data_out = 1;
				serial_port.clk_out = 1;
				during_atn = false;
				printf("*** END OF ATN\n");
				if (listening) {
					// keep holding DATA to indicate we're here
					serial_port.data_out = 0;
					printf("XXX START OF DATA\n");
				} else {
					state = 0;
				}
				break;
			}
			// wait for CLK=1
			if (serial_port.clk_in) {
				serial_port.data_out = 1;
				state = 2;
				valid = true;
				bit = 0;
				byte = 0;
				eoi = false;
				printf("XXX START OF BYTE\n");
			}
			break;
		case 2:
			if (during_atn && !serial_port.atn_in) {
				// cancelled ATN
				serial_port.data_out = 1;
				serial_port.clk_out = 1;
				printf("*** XEND OF ATN\n");
				state = 0;
				break;
			}
			if (valid) {
				// wait for CLK=0, data not valid
				if (!serial_port.clk_in) {
					valid = false;
					printf("XXX NOT VALID\n");
				}
			} else {
				// wait for CLK=1, data valid
				if (serial_port.clk_in) {
					bool b = serial_port.data_in;
					byte |= (b << bit);
					printf("*** BIT%d IN: %d\n", bit, b);
					valid = true;
					if (++bit == 8) {
						printf("*** %s BYTE IN: %02x%s\n", during_atn ? "ATN" : "DATA", byte, eoi ? " (EOI)" : "");
						if (during_atn) {
							if ((byte & 0xe0) == 0x20) {
								if (byte == 0x3f) {
									printf("UNLISTEN\n");
									listening = false;
								} else {
									printf("LISTEN\n");
									listening = true;
								}
							}
						}
						serial_port.data_out = 0;
						state = 1;
					}
				}
			}
			break;
		case 3:
			break;

	}
	printf(">SERIAL IN { ATN:%d CLK:%d DATA:%d } --- OUT { CLK:%d DATA:%d } -- #%d\n", serial_port.atn_in, serial_port.clk_in, serial_port.data_in, serial_port.clk_out, serial_port.data_out, state);
	old_serial_port = serial_port;
}

