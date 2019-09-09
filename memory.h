// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <stdint.h>

uint8_t read6502(uint16_t address);

void memory_save();

void memory_set_ram_bank(uint8_t bank);
void memory_set_rom_bank(uint8_t bank);

uint8_t memory_get_ram_bank();

uint8_t emu_read(uint8_t reg);
void emu_write(uint8_t reg, uint8_t value);

#endif
