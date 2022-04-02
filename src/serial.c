// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include "serial.h"

serial_port_t serial_port;

static int state = 0;

void
serial_step()
{
	static serial_port_t old_serial_port;
	if (old_serial_port.atn_in != serial_port.atn_in ||
		old_serial_port.clk_in != serial_port.clk_in ||
		old_serial_port.data_in != serial_port.data_in) {
		printf("-SERIAL IN { ATN:%d CLK:%d DATA:%d } --- OUT { CLK:%d DATA:%d }\n", old_serial_port.atn_in, old_serial_port.clk_in, old_serial_port.data_in, old_serial_port.clk_out, old_serial_port.data_out);
		printf("+SERIAL IN { ATN:%d CLK:%d DATA:%d } --- OUT { CLK:%d DATA:%d } -- #%d\n", serial_port.atn_in, serial_port.clk_in, serial_port.data_in, serial_port.clk_out, serial_port.data_out, state);

		switch(state) {
			case 0:
				// wait for ATN=1
				if (serial_port.atn_in) {
					serial_port.data_out = 0; // immediately
//					serial_port.clk_out = 1;  // "eventually"
					state = 1;
				}
				break;
			case 1:
				// wait for CLK=1
				if (!serial_port.atn_in) {
					// cancelled ATN
					serial_port.data_out = 1;
					serial_port.clk_out = 1;
					state = 0;
					break;
				}
				if (serial_port.clk_in) {
					serial_port.data_out = 0;
					state = 2;
				}
			case 2:
				break;

		}
		printf(">SERIAL IN { ATN:%d CLK:%d DATA:%d } --- OUT { CLK:%d DATA:%d } -- #%d\n", serial_port.atn_in, serial_port.clk_in, serial_port.data_in, serial_port.clk_out, serial_port.data_out, state);
		old_serial_port = serial_port;
	}
}
