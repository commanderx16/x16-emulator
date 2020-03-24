// Commander X16 Emulator
// Copyright (c) 2020 Michael Steil
// All rights reserved. License: 2-clause BSD

#include "io.h"
#include "memory.h"
#include "ps2.h"
#include "joystick.h"

void
io_init()
{

}

//
// VIA#1
//

// PA0-7 RAM bank
// PB0-2 ROM bank
// PB3   IECATT0
// PB4   IECCLK0
// PB5   IECDAT0
// PB6   IECCLK
// PB7   IECDAT
// CB1   IECSRQ

uint8_t
via1_get_pa()
{
	return 0;
}

void
via1_set_pa(uint8_t pinstate)
{
	memory_set_ram_bank(pinstate);
}

uint8_t
via1_get_pb()
{
	return 0;
}

void
via1_set_pb(uint8_t pinstate)
{
	memory_set_rom_bank(pinstate & 7);
}

bool
via1_get_ca1()
{
	return true; // nothing connected
}

bool
via1_get_cb1()
{
	return true; // nothing connected
}


//
// VIA#2
//

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
	ps2_port[0].data_in = (pinstate >> 0) & 1;
	ps2_port[0].clk_in  = (pinstate >> 1) & 1;
	joystick_latch      = (pinstate >> 3) & 1;
	joystick_clock      = (pinstate >> 5) & 1;
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
	ps2_port[1].data_in = (pinstate >> 0) & 1;
	ps2_port[1].clk_in  = (pinstate >> 1) & 1;
}

bool
via2_get_ca1()
{
	return !(!ps2_port[0].clk_out | !ps2_port[0].clk_in);
}

bool
via2_get_cb1()
{
	return !(!ps2_port[1].clk_out | !ps2_port[1].clk_in);
}

