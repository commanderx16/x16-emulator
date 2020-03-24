// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "via.h"
#include "io.h"
//XXX
#include "glue.h"

#define VIA_IFR_CA2 1
#define VIA_IFR_CA1 2
#define VIA_IFR_SR  4
#define VIA_IFR_CB2 8
#define VIA_IFR_CB1 16
#define VIA_IFR_T2  32
#define VIA_IFR_T1  64

typedef struct {
	uint8_t (*get_pa)();
	void (*set_pa)(uint8_t);
	uint8_t (*get_pb)();
	void (*set_pb)(uint8_t);
	bool (*get_ca1)();
	bool (*get_cb1)();
}  via_iofunc_t;

typedef struct {
	int i; // VIA#

	via_iofunc_t iofunc;

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

via_state_t *
via_create(int i, via_iofunc_t iofunc)
{
	via_state_t *via = calloc(1, sizeof(via_state_t));

	via->i = i;
	via->iofunc = iofunc;

	// output 0
	via->pa_out = 0;
	via->pb_out = 0;

	// DDR to input
	via->ddrb = 0;
	via->ddra = 0;

	via->ier = 0;

	return via;
}

static void
via_state(uint8_t in, uint8_t out, uint8_t ddr, uint8_t *pinstate, uint8_t *readback)
{
	// DDR=0 (input)  -> take input bit
	// DDR=1 (output) -> take output bit

//	// driving state (0 = pulled down, 1 = passive)
//	uint8_t driving = (ddr & out) | ~ddr;
//	printf("driving: %x\n", driving);

	// mix in internal state

	// |   ddr   |   out   |   in    | pinstate |
	// |---------|---------|---------|----------|
	// |    0 in |    0    |    0    |    0     |
	// |    0 in |    0    |    1    |    1     |
	// |    0 in |    1    |    0    |    0     |
	// |    0 in |    1    |    1    |    1     |
	// |    1 out|    0    |    0    |    0     |
	// |    1 out|    0    |    1    |    0     |
	// |    1 out|    1    |    0    |    1     |
	// |    1 out|    1    |    1    |    1     |

	*pinstate =
		(out & ddr) | // VIA    drives wire to 1
		(in & !ddr);  // device drives wire to 1

	// value as read on PA register (*out* will read back our own value)
	*readback = (ddr & out) | (~ddr & *pinstate);
}

void
via_step(via_state_t *via)
{
	uint8_t pa_in = via->iofunc.get_pa();
	via_state(pa_in, via->pa_out, via->ddra, &via->pa_pinstate, &via->pa_readback);
	via->iofunc.set_pa(via->pa_pinstate);

	uint8_t pb_in = via->iofunc.get_pb();
	via_state(pb_in, via->pb_out, via->ddrb, &via->pb_pinstate, &via->pb_readback);
	via->iofunc.set_pb(via->pb_pinstate);
	if (via->i == 1) {
		printf("[%d]: pb_in: %x, via->pb_out: %x, via->ddrb: %x, via->pb_pinstate: %x, via->pb_readback: %x\n", via->i, pb_in, via->pb_out, via->ddrb, via->pb_pinstate, via->pb_readback);
	}

	static bool old_ca1;
	bool ca1 = via->iofunc.get_ca1();
	if (ca1 != old_ca1) {
//		printf("KBD IRQ? CLK is now %d\n", ca1);
		if (!ca1) { // falling edge
			printf("NEW KBD IRQ\n");
			via->ifr |= VIA_IFR_CA1;
		}
	}
	old_ca1 = ca1;

	static bool old_cb1;
	bool cb1 = via->iofunc.get_cb1();
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
via_get_irq_out(via_state_t *via)
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
via_read(via_state_t *via, uint8_t reg)
{
	switch (reg) {
		case 0: // PB
			// reading PB clears clear CB1
			if (via->ifr & VIA_IFR_CB1) {
				printf("clearing IRQ\n");
			}
			via->ifr &= ~VIA_IFR_CB1;

			return via->pb_readback;
		case 1: // PA
			// reading PA clears clear CA1
			//		printf("1CLEAR IRQ\n");
			via->ifr &= ~VIA_IFR_CA1;

			return via->pa_readback;
		case 2: // DDRB
			return via->ddrb;
		case 3: // DDRA
			return via->ddra;
		case 9:
			// timer A and B: return random numbers for RND(0)
			// XXX TODO: these should be real timers :)
			return rand() & 0xff;
		case 13: { // IFR
			uint8_t val = via->ifr;
			if (val) {
				val |= 0x80;
			}
			return val;
		}
		case 14: // IER
			return via->ier;
		default:
			return via->registers[reg];
	}
}

void
via_write(via_state_t *via, uint8_t reg, uint8_t value)
{
	via->registers[reg] = value;

	switch (reg) {
		case 0: // PB
			via->pb_out = value;
			break;
		case 1: // PA
			via->pa_out = value;
			break;
		case 2: // DDRB
			via->ddrb = value;
			break;
		case 3: // DDRBA
			via->ddra = value;
			break;
		case 13: // IFR
			// do nothing
			break;
		case 14: // IER
			if (value & 0x80) { // set
				via->ier |= (value & 0x7f);
			} else { // clear
				via->ier &= ~(value & 0x7f);
			}
		break;
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



static via_state_t *via1;

void
via1_init()
{
	srand(time(NULL));

	via_iofunc_t iofunc = {
		via1_get_pa,
		via1_set_pa,
		via1_get_pb,
		via1_set_pb,
		via1_get_ca1,
		via1_get_cb1
	};
	via1 = via_create(1, iofunc);
}

void
via1_step()
{
	via_step(via1);
}

bool
via1_get_irq_out()
{
	return via_get_irq_out(via1);
}

uint8_t
via1_read(uint8_t reg)
{
	return via_read(via1, reg);
}

void
via1_write(uint8_t reg, uint8_t value)
{
	via_write(via1, reg, value);
}


static via_state_t *via2;

void
via2_init()
{
	via_iofunc_t iofunc = {
		via2_get_pa,
		via2_set_pa,
		via2_get_pb,
		via2_set_pb,
		via2_get_ca1,
		via2_get_cb1
	};
	via2 = via_create(2, iofunc);
}

void
via2_step()
{
	via_step(via2);
}

bool
via2_get_irq_out()
{
	return via_get_irq_out(via2);
}

uint8_t
via2_read(uint8_t reg)
{
	return via_read(via2, reg);
}

void
via2_write(uint8_t reg, uint8_t value)
{
	via_write(via2, reg, value);
}
