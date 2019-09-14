// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "via.h"
#include "ps2.h"
#include "memory.h"
//XXX
#include "glue.h"


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

static uint8_t via1registers[16];

void
via1_init()
{
	srand(time(NULL));
}

uint8_t
via1_read(uint8_t reg)
{
	if (reg == 4 || reg == 5 || reg == 8 || reg == 9) {
		// timer A and B: return random numbers for RND(0)
		// XXX TODO: these should be real timers :)
		return rand() & 0xff;
	} else {
		return via1registers[reg];
	}
}

void
via1_write(uint8_t reg, uint8_t value)
{
	via1registers[reg] = value;
	if (reg == 0) { // PB: ROM bank, IEC
		memory_set_rom_bank(value & 7);
		// TODO: IEC
	} else if (reg == 1) { // PA: RAM bank
		memory_set_ram_bank(value);
	} else {
		// TODO
	}
}

//
// VIA#2
//
// PA0 PS/2 DAT
// PA1 PS/2 CLK
// PA2 LCD backlight
// PA3 NESJOY latch (for both joysticks)
// PA4 NESJOY joy1 data
// PA5 NESJOY joy1 CLK
// PA6 NESJOY joy2 data
// PA7 NESJOY joy2 CLK

static uint8_t via2registers[16];
static uint8_t via2pb_in;

void
via2_init()
{
}

uint8_t
via2_read(uint8_t reg)
{
	if (reg == 0) {
		// PB
		// 0 input  -> take input bit
		// 1 output -> take output bit
		return (via2pb_in & (via2registers[2] ^ 0xff)) |
		       (via2registers[0] & via2registers[2]);
	} else if (reg == 1) {
		// PA
		uint8_t value =
			(via2registers[3] & PS2_CLK_MASK ? 0 : ps2_clk_out << 1) |
			(via2registers[3] & PS2_DATA_MASK ? 0 : ps2_data_out) |
			0x50; // short-circuit NES/SNES contoller -> not present
		return value;
	} else {
		return via2registers[reg];
	}
}

void
via2_write(uint8_t reg, uint8_t value)
{
	via2registers[reg] = value;

	if (reg == 0) {
		// PB
	} else if (reg == 2) {
		// PB DDRB
	} else if (reg == 1 || reg == 3) {
		// PA
		ps2_clk_in = via2registers[3] & PS2_CLK_MASK ? via2registers[1] & PS2_CLK_MASK : 1;
		ps2_data_in = via2registers[3] & PS2_DATA_MASK ? via2registers[1] & PS2_DATA_MASK : 1;
	}
}

uint8_t
via2_pb_get_out()
{
	return via2registers[2] /* DDR  */ & via2registers[0]; /* PB */
}

void
via2_pb_set_in(uint8_t value)
{
	via2pb_in = value;
}

void
via2_sr_set(uint8_t value)
{
	via2registers[10] = value;
}
