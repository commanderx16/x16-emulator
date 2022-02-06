// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "sdcard.h"

bool ss;
bool busy;
bool autotx;
uint8_t sending_byte, received_byte;
int outcounter;

void
vera_spi_init()
{
	ss = false;
	busy = false;
	autotx = false;
	received_byte = 0xff;
}

void
vera_spi_step(int clocks)
{
	if (busy) {
		outcounter += clocks;
		if (outcounter >= 8) {
			busy = false;
			if (sdcard_attached) {
				received_byte = sdcard_handle(sending_byte);
			} else {
				received_byte = 0xff;
			}
		}
	}
}

uint8_t
vera_spi_read(uint8_t reg)
{
	switch (reg) {
		case 0:
			if (autotx && ss && !busy) {
				// autotx mode will automatically send $FF after each read
				sending_byte = 0xff;
				busy = true;
				outcounter = 0;
			}
			return received_byte;
		case 1:
			return busy << 7 | autotx << 3 | ss;
	}
	return 0;
}

void
vera_spi_write(uint8_t reg, uint8_t value)
{
	switch (reg) {
		case 0:
			if (ss && !busy) {
				sending_byte = value;
				busy = true;
				outcounter = 0;
			}
			break;
		case 1:
			if (ss != (value & 1)) {
				ss = value & 1;
				if (ss) {
					sdcard_select(ss);
				}
			}
			autotx = !!(value & 8);
			break;
	}
}
