// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include "via.h"
#include "ps2.h"
#include "i2c.h"
#include "memory.h"
#include "ps2.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//XXX
#include "glue.h"
#include "joystick.h"

//
// VIA#1
//
// PA0: PS2KDAT   PS/2 DATA keyboard
// PA1: PS2KCLK   PS/2 CLK  keyboard
// PA2: NESLATCH  NES LATCH (for all controllers)
// PA3: NESCLK    NES CLK   (for all controllers)
// PA4: NESDAT3   NES DATA  (controller 3)
// PA5: NESDAT2   NES DATA  (controller 2)
// PA6: NESDAT1   NES DATA  (controller 1)
// PA7: NESDAT0   NES DATA  (controller 0)
// PB0: PS2MDAT   PS/2 DATA mouse
// PB1: PS2MCLK   PS/2 CLK  mouse
// PB2: I2CDATA   I2C DATA
// PB3: IECATTO   Serial ATN  out
// PB4: IECCLKO   Serial CLK  out
// PB5: IECDATAO  Serial DATA out
// PB6: IECCLKI   Serial CLK  in
// PB7: IECDATAI  Serial DATA in
// CA1: PS2MCLK   PS/2 CLK  mouse
// CA2: PS2KCLK   PS/2 CLK  keyboard
// CB1: IECSRQ
// CB2: I2CCLK    I2C CLK

static uint8_t via1registers[16];

void
via1_init()
{
	srand(time(NULL));
	i2c_port.clk_in = 1;
}

uint8_t
via1_read(uint8_t reg)
{
	// DDR=0 (input)  -> take input bit
	// DDR=1 (output) -> take output bit
	switch (reg) {
		case 0: // PB
			ps2_autostep(1);
			return (~via1registers[2] & ps2_port[1].out) |
				(via1registers[2] & I2C_DATA_MASK ? 0 : i2c_port.data_out << 2);

		case 1: // PA
			ps2_autostep(0);
			return (~via1registers[3] & ps2_port[0].out) | Joystick_data;

		case 4: // timer
		case 5: // timer
		case 8: // timer
		case 9: // timer
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
	if (reg == 0 || reg == 2) {
		ps2_autostep(1);
		// PB
		const uint8_t pb = via1registers[0] | ~via1registers[2];
		ps2_port[1].in   = pb & PS2_VIA_MASK;
		i2c_port.data_in = via1registers[2] & I2C_DATA_MASK ? via1registers[0] & I2C_DATA_MASK : 1;
	} else if (reg == 1 || reg == 3) {
		ps2_autostep(0);
		// PA
		const uint8_t pa = via1registers[1] | ~via1registers[3];
		ps2_port[0].in   = pa & PS2_VIA_MASK;
		joystick_set_latch(via1registers[1] & JOY_LATCH_MASK);
		joystick_set_clock(via1registers[1] & JOY_CLK_MASK);
	} else if (reg == 12) {
		switch (value >> 5) {
			case 6: // %110xxxxx
				i2c_port.clk_in = 0;
				break;
			case 7: // %111xxxxx
				i2c_port.clk_in = 1;
				break;
		}
	}
}

//
// VIA#2
//
// PA/PB: user port

static uint8_t via2registers[16];

void
via2_init()
{
}

uint8_t
via2_read(uint8_t reg)
{
	return via2registers[reg];
}

void
via2_write(uint8_t reg, uint8_t value)
{
	via2registers[reg] = value;
}
