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

//enum {
//	PS2_STATE_READY_TO,
////	PS2_STATE_
//}

static bool sending = false;
static bool has_byte = false;
static uint8_t current_byte;
static int bit_index = 0;
static int data_bits;
static int send_state = 0;
static int clk_out, data_out;

//#define HOLD 10 /* times 3 cycles = 75 cycles */
#define HOLD 25 /* times 3 cycles = 75 cycles */

void
ps2_step()
{
	int clk_in = via1registers[3] & 0x10 ? via1registers[1] & 0x10 : 1;
	int data_in = via1registers[3] & 0x20 ? via1registers[1] & 0x20 : 1;

//	int old_state = send_state;

	if (!clk_in && data_in) { // communication inhibited
		clk_out = 0;
		data_out = 0;
		sending = false;
		printf("PS2: STATE: communication inhibited.\n");
		goto out;
	} else if (clk_in && data_in) { // idle state
//		printf("PS2: STATE: idle\n");
		if (!sending) {
			// get next byte
			if (!has_byte) {
				current_byte = kbd_buffer_remove();
				if (!current_byte) {
					// we have nothing to send
					clk_out = 1;
					data_out = 0;
					printf("PS2: nothing to send.\n");
					goto out;
				}
				printf("PS2: current_byte: %x\n", current_byte);
				has_byte = true;
			}

			data_bits = current_byte << 1 | (1 - __builtin_parity(current_byte)) << 9 | (1 << 10);
			printf("PS2: data_bits: %x\n", data_bits);
			bit_index = 0;
			send_state = 0;
			sending = true;
		}

		if (send_state <= HOLD) {
			clk_out = 0; // data ready
			data_out = data_bits & 1;
			printf("PS2: [%d]sending #%d: %x\n", send_state, bit_index, data_bits & 1);
			if (send_state == 0 && bit_index == 10) {
				// we have sent the last bit, if the host
				// inhibits now, we'll send the next byte
				has_byte = false;
			}
			if (send_state == HOLD) {
				data_bits >>= 1;
				bit_index++;
			}
			send_state++;
		} else if (send_state <= 2 * HOLD) {
			printf("PS2: [%d]not ready\n", send_state);
			clk_out = 1; // not ready
			data_out = 0;
			if (send_state == 2 * HOLD) {
				printf("XXX bit_index: %d\n", bit_index);
				if (bit_index < 11) {
					send_state = 0;
				} else {
					sending = false;
				}
			}
			if (send_state) {
				send_state++;
			}
		}
	} else {
		printf("PS2: Warning: unknown PS/2 bus state: CLK_IN=%d, DATA_IN=%d\n", clk_in, data_in);
		clk_out = 0;
		data_out = 0;
		goto out;
	}


out:
	return;
//	printf("get_next_ps2_value[%d]: CLK_IN=%d, DATA_IN=%d -> CLK_OUT=%d, DATA_OUT=%d\n", old_state, clk_in, data_in, clk_out, data_out);
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
		uint8_t value =
			(via1registers[3] & 0x10 ? 0 : clk_out << 4) |
			(via1registers[3] & 0x20 ? 0 : data_out << 5);
//		printf("read PB   = $%02x\n", value);
//		printf("read PB   = %s | %s\n", value & 0x10 ? "CLK=1" : "CLK=0", value & 0x20 ? "DATA=1" : "DATA=0");
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
//		printf("set PB   = $%02x\n", value);
//		printf("set PB = %s %s | %s %s\n",
//		       value & 0x10 ? "CLK=1" : "CLK=0",
//		       via1registers[3] & 0x10 ? "" : "(ineffective)",
//		       value & 0x20 ? "DATA=1" : "DATA=0",
//		       via1registers[3] & 0x20 ? "" : "(ineffective)"
//		       );
	} else if (reg == 3) {
//		printf("set DDRB = $%02x\n", value);
		printf("set DDRB = %s | %s\n", value & 0x10 ? "CLK OUT" : "CLK IN", value & 0x20 ? "DATA OUT" : "DATA IN");
	}
	via1registers[reg] = value;
}

