// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include "ps2.h"
#include <stdbool.h>
#include <stdio.h>

#define HOLD 25 * 8 /* 25 x ~3 cycles at 8 MHz = 75µs */

#define PS2_BUFFER_SIZE 32

enum ps2_state {
	PS2_READY,
	PS2_SEND_LO,
	PS2_SEND_HI,
};

struct ring_buffer {
	int     m_oldest;
	int     m_count;
	uint8_t m_elems[PS2_BUFFER_SIZE];
};

static struct {
	enum ps2_state state;
	uint8_t current_byte;
	int data_bits;
	int send_time;
	struct ring_buffer buffer;
} state[2];

ps2_port_t ps2_port[2];

void
ps2_buffer_add(int i, uint8_t byte)
{
	const int index = (state[i].buffer.m_oldest + state[i].buffer.m_count) % PS2_BUFFER_SIZE;
	if (state[i].buffer.m_count >= PS2_BUFFER_SIZE) {
		return;
	}
	++state[i].buffer.m_count;
	state[i].buffer.m_elems[index] = byte;
}

uint8_t
ps2_buffer_pop_oldest(int i)
{
	const uint8_t value      = state[i].buffer.m_elems[state[i].buffer.m_oldest];
	state[i].buffer.m_oldest = (state[i].buffer.m_oldest + 1) % PS2_BUFFER_SIZE;
	--state[i].buffer.m_count;
	return value;
}

void
ps2_step(int i, int clocks)
{
	switch (ps2_port[i].in) {
		case PS2_DATA_MASK:
			// Communication inhibited
			ps2_port[i].out = 0;
			state[i].state  = PS2_READY;
			return;

		case PS2_VIA_MASK:
			// Idle
			switch (state[i].state) {
				case PS2_READY:
				CASE_PS2_READY:
					// get next byte
					if (state[i].data_bits <= 0) {
						if (state[i].buffer.m_count <= 0) {
							// we have nothing to send
							ps2_port[i].out = PS2_CLK_MASK;
							return;
						}
						state[i].current_byte = ps2_buffer_pop_oldest(i);
					}

					state[i].data_bits = state[i].current_byte << 1 | (1 - __builtin_parity(state[i].current_byte)) << 9 | (1 << 10);
					state[i].send_time = 0;
					state[i].state     = PS2_SEND_LO;
					// Fall-thru
				case PS2_SEND_LO:
				CASE_PS2_SEND_LO:
					ps2_port[i].out = state[i].data_bits & 1;
					state[i].send_time += clocks;
					if (state[i].send_time >= HOLD) {
						state[i].data_bits >>= 1;
						state[i].state     = PS2_SEND_HI;
						state[i].send_time = 0;
						clocks -= HOLD;
						goto CASE_PS2_SEND_HI;
					}
					break;
				case PS2_SEND_HI:
				CASE_PS2_SEND_HI:
					ps2_port[i].out = PS2_CLK_MASK; // not ready
					state[i].send_time += clocks;
					if (state[i].send_time >= HOLD) {
						clocks -= HOLD;
						if (state[i].data_bits > 0) {
							state[i].send_time = 0;
							state[i].state     = PS2_SEND_LO;
							goto CASE_PS2_SEND_LO;
						} else {
							state[i].state = PS2_READY;
							goto CASE_PS2_READY;
						}
					}
					break;
			}
			break;
		default:
			ps2_port[i].out = 0;
	}
}

void
ps2_autostep(int i)
{
	static uint32_t port_clocks[2] = {0, 0};
	extern uint32_t clockticks6502;

	int clocks     = clockticks6502 - port_clocks[i];
	port_clocks[i] = clockticks6502;

	ps2_step(i, clocks);
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
	if (PS2_BUFFER_SIZE - state[1].buffer.m_count >= 3) {
		uint8_t byte0 =
		    ((y >> 9) & 1) << 5 |
		    ((x >> 9) & 1) << 4 |
		    1 << 3 |
		    b;
		ps2_buffer_add(1, byte0);
		ps2_buffer_add(1, x);
		ps2_buffer_add(1, y);

		return true;
	} else {
		//		printf("buffer full, skipping...\n");
		return false;
	}
}

void
mouse_send_state(void)
{
	do {
		int send_diff_x = mouse_diff_x > 255 ? 255 : (mouse_diff_x < -256 ? -256 : mouse_diff_x);
		int send_diff_y = mouse_diff_y > 255 ? 255 : (mouse_diff_y < -256 ? -256 : mouse_diff_y);

		mouse_send(send_diff_x, send_diff_y, buttons);

		mouse_diff_x -= send_diff_x;
		mouse_diff_y -= send_diff_y;
	} while (mouse_diff_x != 0 && mouse_diff_y != 0);
}

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
	mouse_diff_x += x;
	mouse_diff_y += y;
}

uint8_t
mouse_read(uint8_t reg)
{
	return 0xff;
}
