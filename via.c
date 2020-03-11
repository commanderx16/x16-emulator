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
#include "joystick.h"

#define VIA_IFR_CA2 1
#define VIA_IFR_CA1 2
#define VIA_IFR_SR  4
#define VIA_IFR_CB2 8
#define VIA_IFR_CB1 16
#define VIA_IFR_T2  32
#define VIA_IFR_T1  64

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

	// default banks are 0
	memory_set_ram_bank(0);
	memory_set_rom_bank(0);
}

void
via1_step()
{
}

bool
via1_get_irq_out()
{
	return false;
}

uint8_t
via1_read(uint8_t reg)
{
	switch (reg) {
		case 0:
			return memory_get_rom_bank(); // PB: ROM bank, IEC
		case 1:
			return memory_get_ram_bank(); // PA: RAM bank
		case 4:
		case 5:
		case 8:
		case 9:
			// timer A and B: return random numbers for RND(0)
			// XXX TODO: these should be real timers :)
			return rand() & 0xff;
		default:
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
static uint8_t via2ifr;
static uint8_t via2ier;

void
via2_init()
{
	via2ier = 0;
}

void
via2_step()
{
	static bool old_clk_out;
	if (ps2_port[0].clk_out != old_clk_out) {
		if (!ps2_port[0].clk_out) { // falling edge
			via2ifr |= VIA_IFR_CA1;
		}
	}
	old_clk_out = ps2_port[0].clk_out;
}

bool
via2_get_irq_out()
{
	return !!(via2ifr & via2ier);
}

uint8_t
via2_read(uint8_t reg)
{
	// DDR=0 (input)  -> take input bit
	// DDR=1 (output) -> take output bit
	if (reg == 0) { // PB
		uint8_t value =
			(via2registers[2] & PS2_CLK_MASK ? 0 : ps2_port[1].clk_out << 1) |
			(via2registers[2] & PS2_DATA_MASK ? 0 : ps2_port[1].data_out);
		return value;
	} else if (reg == 1) { // PA
		// reading PA clears clear CA1
		via2ifr &= ~VIA_IFR_CA1;

		uint8_t value =
			(via2registers[3] & PS2_CLK_MASK ? 0 : ps2_port[0].clk_out << 1) |
			(via2registers[3] & PS2_DATA_MASK ? 0 : ps2_port[0].data_out);
			value = value | (joystick1_data ? JOY_DATA1_MASK : 0) |
							(joystick2_data ? JOY_DATA2_MASK : 0);
		return value;
	} else if (reg == 13) { // IFR
		uint8_t val = via2ifr;
		if (val) {
			val |= 0x80;
		}
		return val;
	} else if (reg == 14) { // IER
		return via2ier;
	} else {
		return via2registers[reg];
	}
}

void
via2_write(uint8_t reg, uint8_t value)
{
	via2registers[reg] = value;

	if (reg == 0 || reg == 2) {
		// PB
		ps2_port[1].clk_in = via2registers[2] & PS2_CLK_MASK ? via2registers[0] & PS2_CLK_MASK : 1;
		ps2_port[1].data_in = via2registers[2] & PS2_DATA_MASK ? via2registers[0] & PS2_DATA_MASK : 1;
	} else if (reg == 1 || reg == 3) {
		// PA
		ps2_port[0].clk_in = via2registers[3] & PS2_CLK_MASK ? via2registers[1] & PS2_CLK_MASK : 1;
		ps2_port[0].data_in = via2registers[3] & PS2_DATA_MASK ? via2registers[1] & PS2_DATA_MASK : 1;
		joystick_latch = via2registers[1] & JOY_LATCH_MASK;
		joystick_clock = via2registers[1] & JOY_CLK_MASK;
	} else if (reg == 13) { // IFR
		// do nothing
	} else if (reg == 14) { // IER
		via2ier = value;
	}

	// reading PA clears clear CA1
	if (reg == 1) { // PA
		via2ifr &= ~VIA_IFR_CA1;
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
