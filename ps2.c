// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "ps2.h"

#define PS2_BUFFER_SIZE 32
struct
{
	uint8_t data[PS2_BUFFER_SIZE];
	uint8_t read;
	uint8_t write;
} ps2_buffer[2];

void
ps2_buffer_add(int i, uint8_t byte)
{
	if ((ps2_buffer[i].write + 1) % PS2_BUFFER_SIZE == ps2_buffer[i].read) {
		// buffer full
		return;
	}

	ps2_buffer[i].data[ps2_buffer[i].write] = byte;
	ps2_buffer[i].write = (ps2_buffer[i].write + 1) % PS2_BUFFER_SIZE;
}

uint8_t
ps2_buffer_remove(int i)
{
	if (ps2_buffer[i].read == ps2_buffer[i].write) {
		return 0; // empty
	} else {
		uint8_t byte = ps2_buffer[i].data[ps2_buffer[i].read];
		ps2_buffer[i].read = (ps2_buffer[i].read + 1) % PS2_BUFFER_SIZE;
		return byte;
	}
}

static bool sending = false;
static bool has_byte = false;
static uint8_t current_byte;
static int bit_index = 0;
static int data_bits;
static int send_state = 0;

#define HOLD 25 * 8 /* 25 x ~3 cycles at 8 MHz = 75µs */

ps2_port_t ps2_port[2];

static int
parity(uint8_t b)
{
	b ^= b >> 4;
	b ^= b >> 2;
	b ^= b >> 1;
	return b & 1;
}

void
ps2_step(int i)
{
	if (!ps2_port[i].clk_in && ps2_port[i].data_in) { // communication inhibited
		ps2_port[i].clk_out = 0;
		ps2_port[i].data_out = 0;
		sending = false;
//		printf("PS2: STATE: communication inhibited.\n");
		return;
	} else if (ps2_port[i].clk_in && ps2_port[i].data_in) { // idle state
//		printf("PS2: STATE: idle\n");
		if (!sending) {
			// get next byte
			if (!has_byte) {
				current_byte = ps2_buffer_remove(i);
				if (!current_byte) {
					// we have nothing to send
					ps2_port[i].clk_out = 1;
					ps2_port[i].data_out = 0;
//					printf("PS2: nothing to send.\n");
					return;
				}
//				printf("PS2: current_byte: %x\n", current_byte);
				has_byte = true;
			}

			data_bits = current_byte << 1 | (1 - parity(current_byte)) << 9 | (1 << 10);
//			printf("PS2: data_bits: %x\n", data_bits);
			bit_index = 0;
			send_state = 0;
			sending = true;
		}

		if (send_state <= HOLD) {
			ps2_port[i].clk_out = 0; // data ready
			ps2_port[i].data_out = data_bits & 1;
//			printf("PS2: [%d]sending #%d: %x\n", send_state, bit_index, data_bits & 1);
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
//			printf("PS2: [%d]not ready\n", send_state);
			ps2_port[i].clk_out = 1; // not ready
			ps2_port[i].data_out = 0;
			if (send_state == 2 * HOLD) {
//				printf("XXX bit_index: %d\n", bit_index);
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
//		printf("PS2: Warning: unknown PS/2 bus state: CLK_IN=%d, DATA_IN=%d\n", ps2_port[i].clk_in, ps2_port[i].data_in);
		ps2_port[i].clk_out = 0;
		ps2_port[i].data_out = 0;
	}
}

// fake mouse

static uint8_t buttons;
static uint16_t mouse_x = 0;
static uint16_t mouse_y = 0;

void
mouse_button_down(int num)
{
	buttons |= 1 << num;
}

void
mouse_button_up(int num)
{
	buttons &= (1 << num) ^ 0xff;
}

void
mouse_move(int x, int y)
{
	mouse_x = x;
	mouse_y = y;
}

uint8_t
mouse_read(uint8_t reg)
{
	switch (reg) {
		case 0:
			return mouse_x & 0xff;
		case 1:
			return mouse_x >> 8;
		case 2:
			return mouse_y & 0xff;
		case 3:
			return mouse_y >> 8;
		case 4:
			return buttons;
		default:
			return 0xff;
	}
}
