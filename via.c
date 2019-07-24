#include <stdio.h>
#include <stdbool.h>
#include "via.h"
#include "ps2.h"
#include "memory.h"
//XXX
#include "glue.h"


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

uint8_t
via1_read(uint8_t reg)
{
	return via1registers[reg];
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
// PA0 PS/2 DAT
// PA1 PS/2 CLK
// PA2 LCD backlight
// PA3 NESJOY latch (for both joysticks)
// PA4 NESJOY joy1 data
// PA5 NESJOY joy1 CLK
// PA6 NESJOY joy2 data
// PA7 NESJOY joy2 CLK

static uint8_t via2registers[16];

uint8_t
via2_read(uint8_t reg)
{
	if (reg == 1) {
		uint8_t value =
			(via1registers[3] & PS2_CLK_MASK ? 0 : ps2_clk_out << 1) |
			(via1registers[3] & PS2_DATA_MASK ? 0 : ps2_data_out);
		return value;
	} else {
		return via2registers[reg];
	}
}

void
via2_write(uint8_t reg, uint8_t value)
{
	via1registers[reg] = value;

	if (reg == 1 || reg == 3) {
		ps2_clk_in = via1registers[3] & PS2_CLK_MASK ? via1registers[1] & PS2_CLK_MASK : 1;
		ps2_data_in = via1registers[3] & PS2_DATA_MASK ? via1registers[1] & PS2_DATA_MASK : 1;
	}
}

