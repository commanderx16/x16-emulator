// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "spi.h"
#include "sdcard.h"
#include "via.h"

// VIA#2
// PB0 SPICLK
// PB1 SS
// PB2-4 unassigned
// PB5 SD write protect
// PB6 SD detect
// PB7 MOSI
// CB1 SPICLK (=PB0)
// CB2 MISO


static bool initialized;

void
spi_init()
{
	initialized = false;
}

void
spi_step()
{
	if (!sdcard_file) {
		return;
	}

	uint8_t port = via2_pb_get_out();
	bool clk = port & 1;
	bool ss = !((port >> 1) & 1);
	bool mosi = port >> 7;

	static bool last_clk = false;
	static bool last_ss;
	static int bit_counter = 0;

	// only care about rising clock
	if (clk == last_clk) {
		return;
	}
	last_clk = clk;
	if (clk == 0) {
		return;
	}

	if (ss && !last_ss) {
		bit_counter = 0;
		sdcard_select();
	}
	last_ss = ss;

	// For initialization, the client has to pull&release CLK 74 times.
	// The SD card should be deselected, because it's not actual
	// data transmission (we ignore this).
	if (!initialized) {
		if (clk == 1) {
			static int init_counter = 0;
			init_counter++;
			if (init_counter >= 70) {
				sdcard_select();
				initialized = true;
			}
		}
		return;
	}

	// for everything else, the SD card neeeds to be selcted
	if (!ss) {
		return;
	}

	// receive byte
	static uint8_t inbyte, outbyte;
	bool bit = mosi;
	inbyte <<= 1;
	inbyte |= bit;
//	printf("BIT: %d BYTE =$%02x\n", bit, inbyte);
	bit_counter++;
	if (bit_counter != 8) {
		return;
	}

	bit_counter = 0;

	if (initialized) {
		outbyte = sdcard_handle(inbyte);
	}

	// send byte
	via2_sr_set(outbyte);
}
