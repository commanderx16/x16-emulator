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

void
get_next_ps2_value(int clk_in, int data_in, int *clk_out, int *data_out)
{
	static bool initialized = false;
	static int byte;

	static int state = 0;
	int old_state = state;

	if (!initialized) {
		initialized = true;
		int value = 0xff;
		byte = value << 1 | (1 - __builtin_parity(value)) << 9 | (1 << 10);
		printf("byte: %x\n", byte);
	}

	switch (state) {
		case 0:
			*clk_out = 1; // not ready
			*data_out = 0;
			state++;
			break;
		case 1:
			*clk_out = 0; // ready
			*data_out = byte & 1;
			state++;
			break;
		case 2:
			*clk_out = 0; // ready
			*data_out = byte & 1;
			byte >>= 1;
			state++;
			break;
		case 3:
			*clk_out = 1; // not ready
			*data_out = 0;
			state = 1;
			break;
	}

	printf("get_next_ps2_value[%d]: CLK_IN=%d, DATA_IN=%d -> CLK_OUT=%d, DATA_OUT=%d\n", old_state, clk_in, data_in, *clk_out, *data_out);
}

uint8_t
via2_read(uint8_t reg)
{
	if (reg == 0) { // PB: fake scan code for now
		uint8_t code = kbd_buffer_remove();
		if (code) {
			//			printf("VIA1PB: $%02x\n", code);
		}
		return code;
	} else if (reg == 1) {
		int clk_in = via1registers[3] & 0x10 ? via1registers[1] & 0x10 : 0;
		int data_in = via1registers[3] & 0x20 ? via1registers[1] & 0x20 : 0;
		int clk_out;
		int data_out;
		get_next_ps2_value(clk_in, data_in, &clk_out, &data_out);
		uint8_t value =
			(via1registers[3] & 0x10 ? 0 : clk_out << 4) |
			(via1registers[3] & 0x20 ? 0 : data_out << 5);
//		printf("read PB   = $%02x\n", value);
		printf("read PB   = %s | %s\n", value & 0x10 ? "CLK=1" : "CLK=0", value & 0x20 ? "DATA=1" : "DATA=0");
		return value;
	} else if (reg == 3) {
//		printf("read DDRB = $%02x\n", via2registers[reg]);
		printf("read DDRB = %s | %s\n", via2registers[reg] & 0x10 ? "CLK OUT" : "CLK IN", via2registers[reg] & 0x20 ? "DATA OUT" : "DATA IN");
		return via2registers[reg];
	} else {
		return via2registers[reg];
	}
}

void
via2_write(uint8_t reg, uint8_t value)
{
	if (reg == 1) {
		printf("set PB   = $%02x\n", value);
		printf("set PB = %s %s | %s %s\n",
		       value & 0x10 ? "CLK=1" : "CLK=0",
		       via1registers[3] & 0x10 ? "" : "(ineffective)",
		       value & 0x20 ? "DATA=1" : "DATA=0",
		       via1registers[3] & 0x20 ? "" : "(ineffective)"
		       );
	} else if (reg == 3) {
//		printf("set DDRB = $%02x\n", value);
		printf("set DDRB = %s | %s\n", value & 0x10 ? "CLK OUT" : "CLK IN", value & 0x20 ? "DATA OUT" : "DATA IN");
	}
	via1registers[reg] = value;
}

