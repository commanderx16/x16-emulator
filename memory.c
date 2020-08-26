// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "glue.h"
#include "via.h"
#include "memory.h"
#include "video.h"
#include "ym2151.h"
#include "ps2.h"
#include "cpu/fake6502.h"

uint8_t ram_bank;
uint8_t rom_bank;

uint8_t *RAM;
uint8_t ROM[ROM_SIZE];

bool led_status;

#define DEVICE_EMULATOR (0x9fb0)

void
memory_init()
{
	RAM = calloc(RAM_SIZE, sizeof(uint8_t));
}

static uint8_t
effective_ram_bank()
{
	return ram_bank % num_ram_banks;
}

//
// interface for fake6502
//
// if debugOn then reads memory only for debugger; no I/O, no side effects whatsoever

uint8_t
read6502(uint16_t address) {
	return real_read6502(address, false, 0);
}

uint8_t
real_read6502(uint16_t address, bool debugOn, uint8_t bank)
{
	if (address < 0x9f00) { // RAM
		return RAM[address];
	} else if (address < 0xa000) { // I/O
		if (address >= 0x9f00 && address < 0x9f20) {
			// TODO: sound
			return 0;
		} else if (address >= 0x9f20 && address < 0x9f40) {
			return video_read(address & 0x1f, debugOn);
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
			return emu_read(address & 0xf, debugOn);
		} else {
			return 0;
		}
	} else if (address < 0xc000) { // banked RAM
		int ramBank = debugOn ? bank % num_ram_banks : effective_ram_bank();
		return	RAM[0xa000 + (ramBank << 13) + address - 0xa000];


	} else { // banked ROM
		int romBank = debugOn ? bank % NUM_ROM_BANKS : rom_bank;
		return ROM[(romBank << 14) + address - 0xc000];
	}
}

void
write6502(uint16_t address, uint8_t value)
{
	static uint8_t lastAudioAdr = 0;
	if (address < 0x9f00) { // RAM
		RAM[address] = value;
	} else if (address < 0xa000) { // I/O
		if (address >= 0x9f00 && address < 0x9f20) {
			// TODO: sound
		} else if (address >= 0x9f20 && address < 0x9f40) {
			video_write(address & 0x1f, value);
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
		} else if (address == 0x9fe0) {
			lastAudioAdr = value;
		} else if (address == 0x9fe1) {
			YM_write_reg(lastAudioAdr, value);
		} else {
			// future expansion
		}
	} else if (address < 0xc000) { // banked RAM
		RAM[0xa000 + (effective_ram_bank() << 13) + address - 0xa000] = value;
	} else { // ROM
		// ignore
	}
}

//
// saves the memory content into a file
//

void
memory_save(SDL_RWops *f, bool dump_ram, bool dump_bank)
{
	if (dump_ram) {
		SDL_RWwrite(f, &RAM[0], sizeof(uint8_t), 0xa000);
	}
	if (dump_bank) {
		SDL_RWwrite(f, &RAM[0xa000], sizeof(uint8_t), (num_ram_banks * 8192));
	}
}


///
///
///

void
memory_set_ram_bank(uint8_t bank)
{
	ram_bank = bank & (NUM_MAX_RAM_BANKS - 1);
}

uint8_t
memory_get_ram_bank()
{
	return ram_bank;
}

void
memory_set_rom_bank(uint8_t bank)
{
	rom_bank = bank & (NUM_ROM_BANKS - 1);;
}

uint8_t
memory_get_rom_bank()
{
	return rom_bank;
}

// Control the GIF recorder
void
emu_recorder_set(gif_recorder_command_t command)
{
	// turning off while recording is enabled
	if (command == RECORD_GIF_PAUSE && record_gif != RECORD_GIF_DISABLED) {
		record_gif = RECORD_GIF_PAUSED; // need to save
	}
	// turning on continuous recording
	if (command == RECORD_GIF_RESUME && record_gif != RECORD_GIF_DISABLED) {
		record_gif = RECORD_GIF_ACTIVE;		// activate recording
	}
	// capture one frame
	if (command == RECORD_GIF_SNAP && record_gif != RECORD_GIF_DISABLED) {
		record_gif = RECORD_GIF_SINGLE;		// single-shot
	}
}

//
// read/write emulator state (feature flags)
//
// 0: debugger_enabled
// 1: log_video
// 2: log_keyboard
// 3: echo_mode
// 4: save_on_exit
// 5: record_gif
// POKE $9FB3,1:PRINT"ECHO MODE IS ON":POKE $9FB3,0
void
emu_write(uint8_t reg, uint8_t value)
{
	bool v = value != 0;
	switch (reg) {
		case 0: debugger_enabled = v; break;
		case 1: log_video = v; break;
		case 2: log_keyboard = v; break;
		case 3: echo_mode = value; break;
		case 4: save_on_exit = v; break;
		case 5: emu_recorder_set((gif_recorder_command_t) value); break;
		case 15: led_status = v; break;
		default: printf("WARN: Invalid register %x\n", DEVICE_EMULATOR + reg);
	}
}

uint8_t
emu_read(uint8_t reg, bool debugOn)
{
	if (reg == 0) {
		return debugger_enabled ? 1 : 0;
	} else if (reg == 1) {
		return log_video ? 1 : 0;
	} else if (reg == 2) {
		return log_keyboard ? 1 : 0;
	} else if (reg == 3) {
		return echo_mode;
	} else if (reg == 4) {
		return save_on_exit ? 1 : 0;
	} else if (reg == 5) {
		return record_gif;

	} else if (reg == 8) {
		return (clockticks6502 >> 0) & 0xff;
	} else if (reg == 9) {
		return (clockticks6502 >> 8) & 0xff;
	} else if (reg == 10) {
		return (clockticks6502 >> 16) & 0xff;
	} else if (reg == 11) {
		return (clockticks6502 >> 24) & 0xff;

	} else if (reg == 13) {
		return keymap;
	} else if (reg == 14) {
		return '1'; // emulator detection
	} else if (reg == 15) {
		return '6'; // emulator detection
	}
	if (!debugOn) printf("WARN: Invalid register %x\n", DEVICE_EMULATOR + reg);
	return -1;
}
