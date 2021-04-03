// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "via.h"
#include "ps2.h"
#include "i2c.h"
#include "memory.h"
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
	if (reg == 0) { // PB
		uint8_t value =
		(via1registers[2] & I2C_DATA_MASK ? 0 : i2c_port.data_out << 2) |
		(via1registers[2] & PS2_CLK_MASK ? 0 : ps2_port[1].clk_out << 1) |
		(via1registers[2] & PS2_DATA_MASK ? 0 : ps2_port[1].data_out);
		return value;
	} else if (reg == 1) { // PA
		uint8_t value =
		(via1registers[3] & PS2_CLK_MASK ? 0 : ps2_port[0].clk_out << 1) |
		(via1registers[3] & PS2_DATA_MASK ? 0 : ps2_port[0].data_out);
		value = value |
		(joystick_data[0] ? JOY_DATA0_MASK : 0) |
		(joystick_data[1] ? JOY_DATA1_MASK : 0) |
		(joystick_data[2] ? JOY_DATA2_MASK : 0) |
		(joystick_data[3] ? JOY_DATA3_MASK : 0);
		return value;
	} else if (reg == 4 || reg == 5 || reg == 8 || reg == 9) { // timer
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

	if (reg == 0 || reg == 2) {
		// PB
		i2c_port.data_in = via1registers[2] & I2C_DATA_MASK ? via1registers[0] & I2C_DATA_MASK : 1;
		ps2_port[1].clk_in = via1registers[2] & PS2_CLK_MASK ? via1registers[0] & PS2_CLK_MASK : 1;
		ps2_port[1].data_in = via1registers[2] & PS2_DATA_MASK ? via1registers[0] & PS2_DATA_MASK : 1;
	} else if (reg == 1 || reg == 3) {
		// PA
		ps2_port[0].clk_in = via1registers[3] & PS2_CLK_MASK ? via1registers[1] & PS2_CLK_MASK : 1;
		ps2_port[0].data_in = via1registers[3] & PS2_DATA_MASK ? via1registers[1] & PS2_DATA_MASK : 1;
		joystick_latch = via1registers[1] & JOY_LATCH_MASK;
		joystick_clock = via1registers[1] & JOY_CLK_MASK;
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

