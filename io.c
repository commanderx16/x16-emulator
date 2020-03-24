// Commander X16 Emulator
// Copyright (c) 2020 Michael Steil
// All rights reserved. License: 2-clause BSD

#include "ps2.h"
#include "joystick.h"

void
io_init()
{

}

uint8_t
via2_get_pa()
{
	return
		(ps2_port[0].data_out << 0) | // PA0 PS/2 DAT
		(ps2_port[0].clk_out  << 1) | // PA1 PS/2 CLK
		(1                    << 2) | // PA2 LCD backlight
		(1                    << 3) | // PA3 NESJOY latch (for both joysticks)
		(joystick1_data       << 4) | // PA4 NESJOY joy1 data
		(1                    << 5) | // PA5 NESJOY CLK (for both joysticks)
		(joystick2_data       << 6) | // PA6 NESJOY joy2 data
		(1                    << 7);  // PA7
}

void
via2_set_pa(uint8_t pinstate)
{
	ps2_port[0].data_in = !!(pinstate >> 0);
	ps2_port[0].clk_in  = !!(pinstate >> 1);
	joystick_latch      = !!(pinstate >> 3);
	joystick_clock      = !!(pinstate >> 5);
}

uint8_t
via2_get_pb()
{
	return
		(ps2_port[1].data_out << 0) | // PB0 PS/2 DAT
		(ps2_port[1].clk_out  << 1) | // PB1 PS/2 CLK
		(1                    << 2) | // PB2
		(1                    << 3) | // PB3
		(1                    << 4) | // PB4
		(1                    << 5) | // PB5
		(1                    << 6) | // PB6
		(1                    << 7);  // PB7
}

void
via2_set_pb(uint8_t pinstate)
{
	ps2_port[1].data_in = !!(pinstate >> 0);
	ps2_port[1].clk_in  = !!(pinstate >> 1);
}

bool
via2_get_ca1()
{
	return ps2_port[0].clk_out;
//	return !!(via2pa_pinstate >> 1);
}

bool
via2_get_cb1()
{
	return ps2_port[1].clk_out;
//	return !!(via2pb_pinstate >> 1);
}

