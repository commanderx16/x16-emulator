// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include "via.h"
#include "i2c.h"
#include "memory.h"
#include "serial.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
//XXX
#include "glue.h"
#include "joystick.h"

typedef struct {
	unsigned timer_count[2];
	unsigned pb6_pulse_counts;
	uint8_t registers[15];
	bool timer1_m1;
	bool timer_running[2];
	bool pb7_output;
} via_t;

static via_t via[2];

// only internal logic is handled here, see via1/2 calls for external
// operations specific to each unit

static void
via_init(via_t *via)
{
	// timer latches, timer counters and SR are not cleared
	for (int i = 0; i < 4; i++) via->registers[i] = 0;
	for (int i = 11; i < 15; i++) via->registers[i] = 0;
	via->timer_running[0] = false;
	via->timer_running[1] = false;
	via->timer1_m1 = false;
	via->pb7_output = true;
}

static void
via_clear_pra_irqs(via_t *via)
{
	via->registers[13] &= ~0x02;
	if ((via->registers[12] & 0b00001010) != 0b00000010) {
		via->registers[13] &= ~0x01;
	}
}

static void
via_clear_prb_irqs(via_t *via)
{
	via->registers[13] &= ~0x10;
	if ((via->registers[12] & 0b10100000) != 0b00100000) {
		via->registers[13] &= ~0x08;
	}
}

static uint8_t
via_read(via_t *via, uint8_t reg, bool debug)
{
	uint8_t ifr;
	bool    irq;
	switch (reg) {
		case 0: // IRB
			if (!debug) via_clear_prb_irqs(via);
			return via->registers[0];
		case 1: // IRA
		case 15:
			if (!debug) via_clear_pra_irqs(via);
			return via->registers[1];
		case 4: // T1L
			if (!debug) via->registers[13] &= ~0x40;
			return (uint8_t)(via->timer_count[0] & 0xff);
		case 5: // T1H
			return (uint8_t)(via->timer_count[0] >> 8);
		case 8: // T2L
			if (!debug) via->registers[13] &= ~0x20;
			return (uint8_t)(via->timer_count[1] & 0xff);
		case 9: // T2H
			return (uint8_t)(via->timer_count[1] >> 8);
		case 10: // SR
			if (!debug) via->registers[13] &= ~0x04;
			return via->registers[10];
		case 13: // IFR
			ifr = via->registers[13];
			irq = (ifr & via->registers[14]) != 0;
			return ((uint8_t)irq << 7) | ifr;
		case 14: // IER
			return via->registers[14] | 0x80;
		default:
			return via->registers[reg];
	}
}

static void
via_write(via_t *via, uint8_t reg, uint8_t value)
{
	uint8_t pcr;
	switch (reg) {
		case 0: // ORB
			via_clear_prb_irqs(via);
			via->registers[0] = value;
			break;
		case 1: // ORA
		case 15:
			via_clear_pra_irqs(via);
			via->registers[1] = value;
			break;
		case 4: // T1L
			via->registers[6] = value;
			break;
		case 5: // T1H
		case 7: // T1LH
			via->registers[13] &= ~0x40;
			via->registers[7] = value;
			if (reg == 5) {
				via->timer_count[0] = ((unsigned)value << 8) | via->registers[6];
				via->timer_running[0] = true;
				via->pb7_output = false;
			}
			break;
		case 9: // T2H
			via->registers[13] &= ~0x20;
			via->timer_count[1] = ((unsigned)value << 8) | via->registers[8];
			via->timer_running[1] = true;
			break;
		case 10: // SR
			via->registers[13] &= ~0x04;
			via->registers[10] = value;
			break;
		case 13: // IFR
			pcr = via->registers[12];
			if ((value & 0x01) && ((pcr & 0b00001010) == 0b00000010)) {
				via->registers[13] &= ~0x01;
			}
			if ((value & 0x08) && ((pcr & 0b10100000) == 0b00100000)) {
				via->registers[13] &= ~0x08;
			}
			break;
		case 14: // IER
			if (value & 0x80) {
				via->registers[14] |= value & 0x7f;
			} else {
				via->registers[14] &= ~value & 0x7f;
			}
			break;
		default:
			via->registers[reg] = value;
	}
}

static void
via_step(via_t *via, unsigned clocks)
{
	// TODO there's currently no timestamp mechanism to mark exact transition
	// times, since there's currently no peripherals that require those
	uint8_t acr = via->registers[11];
	uint8_t ifr = via->registers[13];
	// handle timers
	unsigned cnt;
	uint32_t tclk, tclk_s, reload;
	// counter always update even if it's not "running"
	cnt = via->timer_count[0];
	tclk = clocks;
	while (tclk > 0) {
		if (via->timer1_m1) {
			reload = (((uint32_t)via->registers[7] << 8) | via->registers[6]);
			tclk_s = reload + 1;
			if (tclk < tclk_s) tclk_s = tclk;
			cnt = reload - tclk_s + 1;
			via->timer1_m1 = false;
		} else if (cnt < tclk) {
			if (via->timer_running[0]) {
				ifr |= 0x40;
				via->pb7_output ^= true;
				if (!(acr & 0x40)) via->timer_running[0] = false;
			}
			if (tclk - cnt == 1) {
				// special, -1 state
				cnt = 0xffff;
				via->timer1_m1 = true;
				tclk_s = 1;
			} else {
				reload = (((uint32_t)via->registers[7] << 8) | via->registers[6]);
				tclk_s = cnt + reload + 2;
				if (tclk < tclk_s) tclk_s = tclk;
				cnt += reload - tclk_s + 2;
			}
		} else {
			cnt -= tclk;
			break;
		}
		tclk -= tclk_s;
	}
	via->timer_count[0] = (unsigned)cnt;

	cnt = via->timer_count[1];
	tclk = (acr & 0x20) ? via->pb6_pulse_counts : clocks;
	via->pb6_pulse_counts = 0;
	if (cnt < tclk) {
		if (via->timer_running[1]) {
			ifr |= 0x20;
			via->timer_running[1] = false;
		}
		via->timer_count[1] = 0x10000 + cnt - tclk;
	} else {
		via->timer_count[1] -= tclk;
	}

	// TODO Cxx pin and shift register handling
	
	via->registers[13] = ifr;
}

//
// VIA#1
//
// PA0: I2CDATA   I2C DATA
// PA1: I2CCLK    I2C CLK
// PA2: NESLATCH  NES LATCH (for all controllers)
// PA3: NESCLK    NES CLK   (for all controllers)
// PA4: NESDAT3   NES DATA  (controller 3)
// PA5: NESDAT2   NES DATA  (controller 2)
// PA6: NESDAT1   NES DATA  (controller 1)
// PA7: NESDAT0   NES DATA  (controller 0)
// PB0: -         Unused
// PB1: -         Unused
// PB2: -         Unused
// PB3: IECATTO   Serial ATN  out
// PB4: IECCLKO   Serial CLK  out
// PB5: IECDATAO  Serial DATA out
// PB6: IECCLKI   Serial CLK  in
// PB7: IECDATAI  Serial DATA in
// CA1: -         Unused
// CA2: -         Unused
// CB1: IECSRQ
// CB2: -         Unused

void
via1_init()
{
	via_init(&via[0]);
	i2c_port.clk_in = 1;
	serial_port.in.atn = 0;
	serial_port.in.clk = 0;
	serial_port.in.data = 0;
	serial_port.out.clk = 1;
	serial_port.out.data = 1;
}

uint8_t
via1_read(uint8_t reg, bool debug)
{
	// DDR=0 (input)  -> take input bit
	// DDR=1 (output) -> take output bit
	// For now, just assume that I2C peripherals always drive all lines and VIA
	// overrides them with their input values based on current DDR bits
	switch (reg) {
		case 0: // PB
			if (!debug) via_clear_prb_irqs(&via[0]);
			if (via[0].registers[11] & 2) {
				// TODO latching mechanism (requires IEC implementation)
				return 0;
			} else {
				return
					(~via[0].registers[2] & (
						(serial_port_read_clk() << 6) |
						(serial_port_read_data() << 7)
					)) |
					(via[0].registers[2] & (
						(serial_port.in.atn << 3) |
						((!serial_port.in.clk) << 4) |
						((!serial_port.in.data) << 5)
					));
			}
			
		case 1: // PA
		case 15:
			i2c_step();
			if (!debug) via_clear_pra_irqs(&via[0]);
			if (via[0].registers[11] & 1) {
				// CA1 is currently not connected to anything (?)
				return 0;
			} else {
				return (~via[0].registers[3] & i2c_port.data_out) |					//I2C Data: PA0=1 if DDR bit is 0 (input) and data_out is 1; usage of data_out and data_in is a bit confusing...
					(via[0].registers[3] & i2c_port.data_in) |						//I2C Data: PA0=1 if DDR bit is 1 (output) and data_in is 1
					(~via[0].registers[3] & I2C_CLK_MASK ) |						//I2C Clock: PA1=1 if DDR bit is 0 (input), simulating an input pull-up
					(via[0].registers[3] & i2c_port.clk_in) |						//I2C Clock: PA1=1 if DDR bit is 1 (output) and clk_in is 1, simulating a pin driven by the VIA 
					Joystick_data;
			}

		default:
			return via_read(&via[0], reg, debug);
	}
}

void
via1_write(uint8_t reg, uint8_t value)
{
	via_write(&via[0], reg, value);
	if (reg == 0 || reg == 2) {
		// PB
		const uint8_t pb = via[0].registers[0] | ~via[0].registers[2];
		serial_port.in.atn = (pb & SERIAL_ATNIN_MASK) != 0;
		serial_port.in.clk = (pb & SERIAL_CLOCKIN_MASK) == 0;
		serial_port.in.data = (pb & SERIAL_DATAIN_MASK) == 0;

	} else if (reg == 1 || reg == 3) {
		i2c_step();
		// PA
		const uint8_t pa = via[0].registers[1] | ~via[0].registers[3];
		i2c_port.data_in = pa & I2C_DATA_MASK;										//Sets data_in = 1 if the corresponding DDR bit is 0 (input), simulates a pull-up
		i2c_port.clk_in = (pa & I2C_CLK_MASK) >> 1;									//Sets clk_in = 1 if pin is an input, simulates a pull-up
		joystick_set_latch(via[0].registers[1] & JOY_LATCH_MASK);
		joystick_set_clock(via[0].registers[1] & JOY_CLK_MASK);
	}
}

void
via1_step(unsigned clocks)
{
	via_step(&via[0], clocks);
}

bool
via1_irq()
{
	return (via[0].registers[13] & via[0].registers[14]) != 0;
}

//
// VIA#2
//
// PA/PB: user port
// for now, just assume that all user ports are not connected
// and reads return output register (open bus behavior)

void
via2_init()
{
	via_init(&via[1]);
}

uint8_t
via2_read(uint8_t reg, bool debug)
{
	return via_read(&via[1], reg, debug);
}

void
via2_write(uint8_t reg, uint8_t value)
{
	via_write(&via[1], reg, value);
}

void
via2_step(unsigned clocks)
{
	via_step(&via[1], clocks);
}

bool
via2_irq()
{
	return (via[1].registers[13] & via[1].registers[14]) != 0;
}
