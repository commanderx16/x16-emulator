// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "ps2.h"

#define HOLD 25 * 8 /* 25 x ~3 cycles at 8 MHz = 75µs */

#define PS2_BUFFER_SIZE 32

uint8_t GLOBAL_byte;

static struct {
	bool sending;
	bool has_byte;
	uint8_t current_byte;
	int bit_index;
	int data_bits;
	int send_state;
	struct
	{
		uint8_t data[PS2_BUFFER_SIZE];
		uint8_t read;
		uint8_t write;
	} buffer;
} state[2];

ps2_port_t ps2_port[2];

bool
ps2_buffer_can_fit(int i, int n)
{
	// Math is hard. There's certainly a way to do this without a loop.
	for (int n2 = 1; n2 < n; n2++) {
		if ((state[i].buffer.write + n2) % PS2_BUFFER_SIZE == state[i].buffer.read) {
			return false;
		}
	}
	return true;
}

void
ps2_buffer_add(int i, uint8_t byte)
{
	if (!ps2_buffer_can_fit(i, 1)) {
		return;
	}

	state[i].buffer.data[state[i].buffer.write] = byte;
	state[i].buffer.write = (state[i].buffer.write + 1) % PS2_BUFFER_SIZE;
}

int
ps2_buffer_remove(int i)
{
	if (state[i].buffer.read == state[i].buffer.write) {
		return -1; // empty
	} else {
		uint8_t byte = state[i].buffer.data[state[i].buffer.read];
		state[i].buffer.read = (state[i].buffer.read + 1) % PS2_BUFFER_SIZE;
		GLOBAL_byte = byte;
		return byte;
	}
}

void
ps2_init(int i)
{
	ps2_port[i].clk_out = 1;
	ps2_port[i].data_out = 1;
}

//int ps2_debug = 1 | 2;
int ps2_debug = 0;

#define PS2_DEBUG (ps2_debug & (1 << i))

void
ps2_step(int i)
{
	// if we're pulling a wire to 0, assume it's 1
	bool clk_in = (!ps2_port[i].clk_out) ? 1 : ps2_port[i].clk_in;
	bool data_in = (!ps2_port[i].data_out) ? 1 : ps2_port[i].data_in;

//	if (PS2_DEBUG) printf("PS2[%d]: %d/%d\n", i, clk_in, data_in);
	if (!clk_in && data_in) {
		// CLK=0, DATA=1: communication inhibited
		ps2_port[i].clk_out = 1;
		ps2_port[i].data_out = 1;
		state[i].sending = false;
		if (PS2_DEBUG) printf("PS2[%d]: STATE: communication inhibited.\n", i);
		return;
	} else if (clk_in && data_in) {
		// CLK=1, DATA=1: idle state
//		printf("PS2[%d]: STATE: idle\n", i);
		if (!state[i].sending) {
			// get next byte
			if (!state[i].has_byte) {
				int current_byte = ps2_buffer_remove(i);
				if (current_byte < 0) {
					// we have nothing to send
					ps2_port[i].clk_out = 1;
					ps2_port[i].data_out = 1;
//					if (PS2_DEBUG) printf("PS2[%d]: nothing to send.\n", i);
					return;
				}
				state[i].current_byte = current_byte;
				if (PS2_DEBUG) printf("PS2[%d]: current_byte: %x\n", i, state[i].current_byte);
				state[i].has_byte = true;
			}

			state[i].data_bits = state[i].current_byte << 1 | (1 - __builtin_parity(state[i].current_byte)) << 9 | (1 << 10);
			if (PS2_DEBUG) {
				printf("PS2[%d]: data_bits: ", i);
				for (int j = 10; j >= 0; j--) {
					if (j == 9 || j == 8 || j == 0) {
						printf("-");
					}
					printf("%d", (state[i].data_bits >> j) & 1);
				}
				printf("(0x%x)\n", state[i].data_bits);
			}
			state[i].bit_index = 0;
			state[i].send_state = 0;
			state[i].sending = true;
		}

		if (state[i].send_state <= HOLD) {
			ps2_port[i].clk_out = 0; // data ready
			ps2_port[i].data_out = state[i].data_bits & 1;
			if (state[i].send_state == 0) if (PS2_DEBUG) printf("PS2[%d]: [%d]sending #%d: %x\n", i, state[i].send_state, state[i].bit_index, state[i].data_bits & 1);
			if (state[i].send_state == 0 && state[i].bit_index == 10) {
				// we have sent the last bit, if the host
				// inhibits now, we'll send the next byte
				state[i].has_byte = false;
			}
			if (state[i].send_state == HOLD) {
				state[i].data_bits >>= 1;
				state[i].bit_index++;
			}
			state[i].send_state++;
		} else if (state[i].send_state <= 2 * HOLD) {
			if (state[i].send_state == HOLD + 1) if (PS2_DEBUG) printf("PS2[%d]: [%d]not ready\n", i, state[i].send_state);
			ps2_port[i].clk_out = 1; // not ready
			ps2_port[i].data_out = 0;
			if (state[i].send_state == 2 * HOLD) {
//				printf("XXX bit_index: %d\n", state[i].bit_index);
				if (state[i].bit_index < 11) {
					state[i].send_state = 0;
				} else {
					state[i].sending = false;
				}
			}
			if (state[i].send_state) {
				state[i].send_state++;
			}
		}
	} else if (!clk_in && !data_in) {
		// CLK=0, DATA=0: reset state
		if (PS2_DEBUG) printf("PS2[%d]: RESET state\n", i);
	} else {
		if (PS2_DEBUG) printf("PS2[%d]: Warning: unknown PS/2 bus state: CLK_IN=%d, DATA_IN=%d\n", i, clk_in, data_in);
		ps2_port[i].clk_out = 1;
		ps2_port[i].data_out = 1;
	}
}

// fake mouse

static uint8_t buttons;
static int16_t mouse_diff_x = 0;
static int16_t mouse_diff_y = 0;

// byte 0, bit 7: Y overflow
// byte 0, bit 6: X overflow
// byte 0, bit 5: Y sign bit
// byte 0, bit 4: X sign bit
// byte 0, bit 3: Always 1
// byte 0, bit 2: Middle Btn
// byte 0, bit 1: Right Btn
// byte 0, bit 0: Left Btn
// byte 2:        X Movement
// byte 3:        Y Movement


static bool
mouse_send(int x, int y, int b)
{
	if (ps2_buffer_can_fit(1, 3)) {
		uint8_t byte0 =
			((y >> 9) & 1) << 5 |
			((x >> 9) & 1) << 4 |
			1 << 3 |
			b;
		uint8_t byte1 = x;
		uint8_t byte2 = y;
//		printf("%02X %02X %02X\n", byte0, byte1, byte2);

		ps2_buffer_add(1, byte0);
		ps2_buffer_add(1, byte1);
		ps2_buffer_add(1, byte2);

		return true;
	} else {
//		printf("buffer full, skipping...\n");
		return false;
	}
}

void
mouse_send_state()
{
	if (mouse_diff_x > 255) {
		mouse_send(255, 0, buttons);
		mouse_diff_x -= 255;
	}
	if (mouse_diff_x < -256) {
		mouse_send(-256, 0, buttons);
		mouse_diff_x -= -256;
	}
	if (mouse_diff_y > 255) {
		mouse_send(0, 255, buttons);
		mouse_diff_y -= 255;
	}
	if (mouse_diff_y < -256) {
		mouse_send(0, -256, buttons);
		mouse_diff_y -= -256;
	}
	if (mouse_send(mouse_diff_x, mouse_diff_y, buttons)) {
		mouse_diff_x = 0;
		mouse_diff_y = 0;
	}
}


void
mouse_button_down(int num)
{
	buttons |= 1 << num;
	mouse_send_state();
}

void
mouse_button_up(int num)
{
	buttons &= (1 << num) ^ 0xff;
	mouse_send_state();
}

void
mouse_move(int x, int y)
{
	mouse_diff_x += x;
	mouse_diff_y += y;
	mouse_send_state();
}

uint8_t
mouse_read(uint8_t reg)
{
	return 0xff;
}

