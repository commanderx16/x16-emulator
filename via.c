// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "via.h"
#include "io.h"
#include "memory.h"
//XXX
#include "glue.h"

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

typedef struct {
	uint8_t registers[16];

	uint8_t ifr;
	uint8_t ier;

	uint8_t pa_out;
	uint8_t pb_out;
	uint8_t pa_pinstate;
	uint8_t pb_pinstate;
	uint8_t pa_readback;
	uint8_t pb_readback;
	uint8_t ddra;
	uint8_t ddrb;
}  via_state_t;

static via_state_t *via;

void
via_init()
{
	via = calloc(1, sizeof(via_state_t));

	via->ier = 0;

	// DDR to input
	via->ddrb = 0;
	via->ddra = 0;
}

void
via_state(uint8_t in, uint8_t out, uint8_t ddr, uint8_t *pinstate, uint8_t *readback)
{
	// DDR=0 (input)  -> take input bit
	// DDR=1 (output) -> take output bit

	// driving state (0 = pulled, 1 = passive)
	uint8_t driving = (ddr & out) | ~ddr;

	// mix in internal state (open collector)
	*pinstate = ~(~in | ~driving);

	// value as read on PA register (*out* will read back our own value)
	*readback = (ddr & driving) | (~ddr & *pinstate);
}

void
via_step()
{
	uint8_t pa_in = via2_get_pa();
	via_state(pa_in, via->pa_out, via->ddra, &via->pa_pinstate, &via->pa_readback);
	via2_set_pa(via->pa_pinstate);

	uint8_t pb_in = via2_get_pb();
	via_state(pb_in, via->pb_out, via->ddrb, &via->pb_pinstate, &via->pb_readback);
	via2_set_pb(via->pb_pinstate);

	static bool old_ca1;
	bool ca1 = via2_get_ca1();
	if (ca1 != old_ca1) {
//		printf("KBD IRQ? CLK is now %d\n", ca1);
		if (!ca1) { // falling edge
			printf("NEW KBD IRQ\n");
			via->ifr |= VIA_IFR_CA1;
		}
	}
	old_ca1 = ca1;

	static bool old_cb1;
	bool cb1 = via2_get_cb1();
	if (cb1 != old_cb1) {
//		printf("MSE IRQ? CLK is now %d\n", cb1);
		if (!cb1) { // falling edge
			printf("NEW MSE IRQ\n");
			via->ifr |= VIA_IFR_CB1;
		}
	}
	old_cb1 = cb1;
}

bool
via_get_irq_out()
{
//	if (!!(via->ifr & via->ier)) printf("YYY %d\n", !!(via->ifr & via->ier));
	static int count;
	if (((via->ifr & via->ier) & (VIA_IFR_CA1 | VIA_IFR_CB1)) == (VIA_IFR_CA1 | VIA_IFR_CB1)) {
		printf("BOTH SOURCES!\n");
		count++;
		if (count > 100) {
			printf("BOTHBOTH!\n");
		}
	} else {
		count = 0;
	}
	return !!(via->ifr & via->ier);
}

uint8_t
via_read(uint8_t reg)
{
	if (reg == 0) { // PB
		// reading PB clears clear CB1
		if (via->ifr & VIA_IFR_CB1) {
			printf("clearing IRQ\n");
		}
		via->ifr &= ~VIA_IFR_CB1;

		return via->pb_readback;
	} else if (reg == 1) { // PA
		// reading PA clears clear CA1
//		printf("1CLEAR IRQ\n");
		via->ifr &= ~VIA_IFR_CA1;

		return via->pa_readback;
	} else if (reg == 2) { // DDRB
		return via->ddrb;
	} else if (reg == 3) { // DDRA
		return via->ddra;
	} else if (reg == 13) { // IFR
		uint8_t val = via->ifr;
		if (val) {
			val |= 0x80;
		}
		return val;
	} else if (reg == 14) { // IER
		return via->ier;
	} else {
		return via->registers[reg];
	}
}

void
via_write(uint8_t reg, uint8_t value)
{
	via->registers[reg] = value;

	if (reg == 0) { // PB
		via->pb_out = value;
	} else if (reg == 1) { // PA
		via->pa_out = value;
	} else if (reg == 2) { // DDRB
		via->ddrb = value;
	} else if (reg == 3) { // DDRBA
		via->ddra = value;
	} else if (reg == 13) { // IFR
		// do nothing
	} else if (reg == 14) { // IER
		if (value & 0x80) { // set
			via->ier |= (value & 0x7f);
		} else { // clear
			via->ier &= ~(value & 0x7f);
		}
	}

	// reading PB clears clear CB1
	if (reg == 0) { // PA
		via->ifr &= ~VIA_IFR_CB1;
	}

	// reading PA clears clear CA1
	if (reg == 1) { // PA
//		printf("2CLEAR IRQ\n");
		via->ifr &= ~VIA_IFR_CA1;
	}
}

bool
via2_get_irq_out()
{
	return via_get_irq_out();
}

void
via2_init()
{
	return via_init();
}
uint8_t
via2_read(uint8_t reg)
{
	return via_read(reg);
}

void
via2_write(uint8_t reg, uint8_t value)
{
	via_write(reg, value);
}

void
via2_step()
{
	via_step();
}
