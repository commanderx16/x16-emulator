// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "sdcard.h"

bool ss;
bool busy;
uint8_t sending_byte, received_byte;
int outcounter;

void
vera_spi_init()
{
	ss = false;
	busy = false;
	received_byte = 0xff;
}

void
vera_spi_step()
{
	if (busy) {
		outcounter++;
		if (outcounter == 8) {
			busy = false;
			received_byte = sdcard_handle(sending_byte);
		}
	}
}

uint8_t
vera_spi_read(uint8_t reg)
{
	switch (reg) {
		case 0:
			return received_byte;
		case 1:
			return busy << 7 | ss;
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
					sdcard_select();
				}
			}
			break;
	}
}
