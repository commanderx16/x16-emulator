#include <sys/types.h>
#include <string.h>
#include "glue.h"
#include "memory.h"
#include "video.h"

uint16_t ram_bot, ram_top;

//
// interface for fake6502
//

uint8_t RAM[65536];

uint8_t
read6502(uint16_t address)
{
	if (address >= 0x9f00 && address < 0xa000) {
		// I/O
		if (address >= 0x9f00 && address < 0x9f20) {
			// TODO: sound
			return 0;
		} else if (address >= 0x9f20 && address < 0x9f28) {
			return video_read(address & 7);
		} else if (address >= 0x9f40 && address < 0x9f60) {
			// TODO: character LCD
			return 0;
		} else if (address >= 0x9f60 && address < 0x9f70) {
			// TODO: VIA #1
			return 0;
		} else if (address >= 0x9f70 && address < 0x9f80) {
			// TODO: VIA #2
			return 0;
		} else if (address >= 0x9f80 && address < 0x9fa0) {
			// TODO: RTC
			return 0;
		} else {
			return 0;
		}
	} else {
		// RAM/ROM
		return RAM[address];
	}
}

void
write6502(uint16_t address, uint8_t value)
{
	if (address < 0x9f00) {
		RAM[address] = value;
	} else {
		if (address >= 0x9f00 && address < 0x9f20) {
			// TODO: sound
		} else if (address >= 0x9f20 && address < 0x9f40) {
			video_write(address & 7, value);
		} else if (address >= 0x9f40 && address < 0x9f60) {
			// TODO: character LCD
		} else if (address >= 0x9f60 && address < 0x9f70) {
			// TODO: VIA #1
		} else if (address >= 0x9f70 && address < 0x9f80) {
			// TODO: VIA #2
		} else if (address >= 0x9f80 && address < 0x9fa0) {
			// TODO: RTC
		} else {
			// future expansion
		}
	}
}

// RAMTAS - Perform RAM test
void
RAMTAS()
{
	ram_bot = 0x0800;
	ram_top = 0xA000;

	// clear zero page
	memset(RAM, 0, 0x100);
	// clear 0x200-0x3ff
	memset(&RAM[0x0200], 0, 0x200);
}

// MEMTOP - Read/set the top of memory
void
MEMTOP()
{
	if (status & 1) {
		x = ram_top & 0xFF;
		y = ram_top >> 8;
	} else {
		ram_top = x | (y << 8);
	}
}

// MEMBOT - Read/set the bottom of memory
void
MEMBOT()
{
	if (status & 1) {
		x = ram_bot & 0xFF;
		y = ram_bot >> 8;
	} else {
		ram_bot = x | (y << 8);
	}
}
