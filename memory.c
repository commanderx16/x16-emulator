// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "glue.h"
#include "via.h"
#include "memory.h"
#include "video.h"
#include "ps2.h"

uint8_t ram_bank = NUM_RAM_BANKS - 1;
uint8_t rom_bank = NUM_ROM_BANKS - 1;

uint8_t RAM[RAM_SIZE];
uint8_t ROM[ROM_SIZE];

#define DEVICE_EMULATOR (0x9fb0)

// ROM file layout:
//   0000-1FFF: bank 0       ($C000)
//   2000-3FFF: fixed KERNAL ($E000)
//   4000-5FFF: bank 1       ($C000)
//   6000-7FFF: bank 2       ($C000)
//   ...

//
// interface for fake6502
//

uint8_t
read6502(uint16_t address)
{
	if (address < 0x9f00) { // RAM
		return RAM[address];
	} else if (address < 0xa000) { // I/O
		if (address >= 0x9f00 && address < 0x9f20) {
			// TODO: sound
			return 0;
		} else if (address >= 0x9f20 && address < 0x9f28) {
			return video_read(address & 7);
		} else if (address >= 0x9f40 && address < 0x9f60) {
			// TODO: character LCD
			return 0;
		} else if (address >= 0x9f60 && address < 0x9f70) {
			return via1_read(address & 0xf);
		} else if (address >= 0x9f70 && address < 0x9f80) {
			return via2_read(address & 0xf);
		} else if (address >= 0x9f80 && address < 0x9fa0) {
			// TODO: RTC
			return 0;
		} else if (address >= 0x9fa0 && address < 0x9fb0) {
			// fake mouse
			return mouse_read(address & 0x1f);
		} else if (address >= 0x9fb0 && address < 0x9fc0) {
			// emulator state
			return emu_read(address & 0xf);
		} else {
			return 0;
		}
	} else if (address < 0xc000) { // banked RAM
		return RAM[0xa000 + (ram_bank << 13) + address - 0xa000];
	} else if (address < 0xe000) { // banked ROM
		if (rom_bank == 0) {
			// BASIC is at offset 0 * 8192 in ROM
			return ROM[address - 0xc000];
		} else {
			// other banks are at offset (n + 1) * 8192 in ROM
			return ROM[((rom_bank + 1) << 13) + address - 0xc000];
		}
	} else { // fixed ROM
		// KERNAL is at offset 1 * 8192 in ROM
		return ROM[address - 0xe000 + 0x2000];
	}
}

void
write6502(uint16_t address, uint8_t value)
{
	if (address < 0x9f00) { // RAM
		RAM[address] = value;
	} else if (address < 0xa000) { // I/O
		if (address >= 0x9f00 && address < 0x9f20) {
			// TODO: sound
		} else if (address >= 0x9f20 && address < 0x9f40) {
			video_write(address & 7, value);
		} else if (address >= 0x9f40 && address < 0x9f60) {
			// TODO: character LCD
		} else if (address >= 0x9f60 && address < 0x9f70) {
			via1_write(address & 0xf, value);
		} else if (address >= 0x9f70 && address < 0x9f80) {
			via2_write(address & 0xf, value);
		} else if (address >= 0x9f80 && address < 0x9fa0) {
			// TODO: RTC
		} else if (address >= 0x9fb0 && address < 0x9fc0) {
			// emulator state
			emu_write(address & 0xf, value);
		} else {
			// future expansion
		}
	} else if (address < 0xc000) { // banked RAM
		RAM[0xa000 + (ram_bank << 13) + address - 0xa000] = value;
	} else { // ROM
		// ignore
	}
}

//
//
//

void
memory_save()
{
	int index = 0;
	char filename[22];
	for (;;) {
		if (!index) {
			strcpy(filename, "memory.bin");
		} else {
			sprintf(filename, "memory-%i.bin", index);
		}
		if (access(filename, F_OK) == -1) {
			break;
		}
		index++;
	}
	FILE *f = fopen(filename, "wb");
	if (!f) {
		printf("Cannot write to %s!\n", filename);
		return;
	}
	fwrite(RAM, RAM_SIZE, 1, f);
	fclose(f);
	printf("Saved memory contents to %s.\n", filename);
}

//
//
//

void
memory_set_ram_bank(uint8_t bank)
{
	ram_bank = bank & (NUM_RAM_BANKS - 1);
}

uint8_t memory_get_ram_bank()
{
	return ram_bank;
}

void
memory_set_rom_bank(uint8_t bank)
{
	rom_bank = bank & (NUM_ROM_BANKS - 1);;
}

//
// read/write emulator state (feature flags)
//
// 0: debuger_enabled
// 1: log_video
// 2: log_keyboard
// 3: echo_mode
// 4: save_on_exit
// POKE $9FB3,1:PRINT"ECHO MODE IS ON":POKE $9FB3,0
void
emu_write(uint8_t reg, uint8_t value)
{
	bool v = value != 0;
	switch (reg) {
		case 0: debuger_enabled = v; break;
		case 1: log_video = v; break;
		case 2: log_keyboard = v; break;
		case 3: echo_mode = v; break;
		case 4: save_on_exit = v; break;
		default: printf("WARN: Invalid register %x\n", DEVICE_EMULATOR + reg);
	}
}

uint8_t
emu_read(uint8_t reg)
{
	if (reg == 0) {
		return debuger_enabled ? 1 : 0;
	} else if (reg == 1) {
		return log_video ? 1 : 0;
	} else if (reg == 2) {
		return log_keyboard ? 1 : 0;
	} else if (reg == 3) {
		return echo_mode ? 1 : 0;
	} else if (reg == 4) {
		return save_on_exit ? 1 : 0;
	} else if (reg == 13) {
		return keymap;
	} else if (reg == 14) {
		return '1'; // emulator detection
	} else if (reg == 15) {
		return '6'; // emulator detection
	}
	printf("WARN: Invalid register %x\n", DEVICE_EMULATOR + reg);
	return -1;
}
