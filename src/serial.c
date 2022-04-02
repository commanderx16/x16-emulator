// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include "serial.h"

serial_port_t serial_port;

void
serial_step()
{
	static serial_port_t old_serial_port;
	if (old_serial_port.atn_in != serial_port.atn_in ||
		old_serial_port.clk_in != serial_port.clk_in ||
		old_serial_port.data_in != serial_port.data_in) {
//		printf("-SERIAL ATN:%d CLK:%d DATA:%d\n", old_serial_port.atn_in, old_serial_port.clk_in, old_serial_port.data_in);
		printf("+SERIAL ATN:%d CLK:%d DATA:%d\n", serial_port.atn_in, serial_port.clk_in, serial_port.data_in);
		old_serial_port = serial_port;
	}
}
