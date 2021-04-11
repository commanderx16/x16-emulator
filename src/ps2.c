// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include "ps2.h"
#include <stdbool.h>
#include <stdio.h>

#define HOLD 25 * 8 /* 25 x ~3 cycles at 8 MHz = 75µs */

#define PS2_BUFFER_SIZE 32

enum ps2_mode {
	PS2_MODE_DETECT,
	PS2_MODE_INHIBITED,
	PS2_MODE_RECEIVING,
	PS2_MODE_SENDING,
};

enum ps2_state {
	PS2_READY,
	PS2_SEND_LO,
	PS2_SEND_HI,
	PS2_RECV_HI,
	PS2_RECV_LO,
};

struct ring_buffer {
	int     m_oldest;
	int     m_count;
	uint8_t m_elems[PS2_BUFFER_SIZE];
};

static struct {
	enum ps2_mode mode;
	enum ps2_state state;
	uint8_t current_byte;
	int data_bits;
	int count;
	int send_time;
	struct ring_buffer outbuffer;
	struct ring_buffer inbuffer;
} state[2];

ps2_port_t ps2_port[2];

void
buffer_add(struct ring_buffer *buffer, uint8_t byte)
{
	const int index = (buffer->m_oldest + buffer->m_count) % PS2_BUFFER_SIZE;
	if (buffer->m_count >= PS2_BUFFER_SIZE) {
		return;
	}
	buffer->m_count++;
	buffer->m_elems[index] = byte;
}

uint8_t
buffer_get_count(struct ring_buffer *buffer)
{
	return buffer->m_count;
}

uint8_t
buffer_get_oldest(struct ring_buffer *buffer)
{
	return buffer->m_elems[buffer->m_oldest];
}

uint8_t
buffer_pop_oldest(struct ring_buffer *buffer)
{
	const uint8_t value = buffer_get_oldest(buffer);
	buffer->m_oldest = (buffer->m_oldest + 1) % PS2_BUFFER_SIZE;
	buffer->m_count--;
	return value;
}

void
ps2_outbuffer_add(int i, uint8_t byte)
{
	buffer_add(&state[i].outbuffer, byte);
}

uint8_t
ps2_outbuffer_get_count(int i)
{
	return buffer_get_count(&state[i].outbuffer);
}

uint8_t
ps2_outbuffer_pop_oldest(int i)
{
	return buffer_pop_oldest(&state[i].outbuffer);
}

void
ps2_inbuffer_add(int i, uint8_t byte)
{
	buffer_add(&state[i].inbuffer, byte);
}

uint8_t
ps2_inbuffer_get_count(int i)
{
	return buffer_get_count(&state[i].inbuffer);
}

uint8_t
ps2_inbuffer_pop_oldest(int i)
{
	return buffer_pop_oldest(&state[i].inbuffer);
}

int bit;

void
ps2_step(int i, int clocks)
{
	// XXX
	if (i != 0) {
		return;
	}

	for (;;) {
		state[i].send_time += clocks;
		if (state[i].send_time < HOLD) {
			return;
		}
		state[i].send_time -= HOLD;
		clocks -= HOLD;

		if (state[i].mode == PS2_MODE_DETECT || state[i].mode == PS2_MODE_INHIBITED) {
			switch (ps2_port[i].in) {
				default:                           // DATA=0, CLK=0
				case PS2_DATA_MASK:                // DATA=1, CLK=0
					printf("** Communication inhibited %i\n", i);
					state[i].mode = PS2_MODE_INHIBITED;
					break;
				case PS2_CLK_MASK:                 // DATA=0, CLK=1
					printf("** Host Request-to-Send %i\n", i);
					state[i].mode = PS2_MODE_RECEIVING;
					state[i].state = PS2_RECV_LO;
					state[i].count = 0;
					break;
				case PS2_DATA_MASK | PS2_CLK_MASK: // DATA=1, CLK=1
					printf("** Idle %i\n", i);
					state[i].data_bits = 0;
					state[i].mode = PS2_MODE_SENDING;
					state[i].state = PS2_READY;
					break;
			}
		}

		switch (state[i].mode) {
			case PS2_MODE_INHIBITED:
				printf("Communication inhibited\n");
				// Communication inhibited
				ps2_port[i].out = PS2_DATA_MASK | PS2_CLK_MASK; // CLK=1 DATA=1
				break;

			case PS2_MODE_RECEIVING:
	//			printf("Host Request-to-Send\n");
				// Host Request-to-Send
				switch (state[i].state) {
					case PS2_RECV_HI:
						printf("RECV HI transition %d\n", state[i].count);
						if (state[i].count == 10) {
							printf("ACK end\n");
							ps2_port[i].out = PS2_DATA_MASK | PS2_CLK_MASK; // CLK=1
							state[i].mode = PS2_MODE_DETECT;
							break;
						}
						state[i].data_bits |= ((ps2_port[i].in & PS2_DATA_MASK) ? 1 : 0) << state[i].count;
						printf("BIT%d: %d -> $%02X\n", state[i].count, (ps2_port[i].in & PS2_DATA_MASK) ? 1 : 0, state[i].data_bits);
						ps2_port[i].out = PS2_DATA_MASK | PS2_CLK_MASK; // CLK=1
						state[i].state = PS2_RECV_LO;
						state[i].count++;
						break;
					case PS2_RECV_LO:
						printf("RECV LO transition %d\n", state[i].count);
						if (state[i].count == 10) {
							bool parity_ok = !__builtin_parity(state[i].data_bits & 0x1ff);
							bool stop_ok = !!(state[i].data_bits & 0x200);
							printf("ACK BYTE $%02X (%d/%d)\n", state[i].data_bits & 0xff, parity_ok, stop_ok);
							ps2_port[i].out = 0;             // CLK=0, DATA=0
						} else {
							ps2_port[i].out = PS2_DATA_MASK; // CLK=0, DATA=1
						}
						state[i].state = PS2_RECV_HI;
						break;
					default:
						printf("XXX");
						break;
				}
				break;

			case PS2_MODE_SENDING:
				printf("Idle\n");
				// Idle
				switch (state[i].state) {
					case PS2_READY:
						printf("PS2_READY\n");
						// get next byte
						if (state[i].data_bits <= 0) {
							if (ps2_outbuffer_get_count(i) <= 0) {
								// we have nothing to send
								ps2_port[i].out = PS2_CLK_MASK;
								return;
							}
							state[i].current_byte = ps2_outbuffer_pop_oldest(i);
						}

						state[i].data_bits = state[i].current_byte << 1 | (1 - __builtin_parity(state[i].current_byte)) << 9 | (1 << 10);
						state[i].state = PS2_SEND_LO;
						break;
					case PS2_SEND_LO:
						printf("SEND LO transition\n");
						ps2_port[i].out = (state[i].data_bits & 1) ? PS2_DATA_MASK : 0; // CLK=0 DATA=bit
						state[i].data_bits >>= 1;
						state[i].state = PS2_SEND_HI;
						break;
					case PS2_SEND_HI:
						printf("SEND HI transition\n");
						ps2_port[i].out |= PS2_CLK_MASK; // CLK=1: not ready
						if (state[i].data_bits != 0) {
							state[i].state = PS2_SEND_LO;
							break;
						} else {
							state[i].mode = PS2_MODE_DETECT;
							break;
						}
					default:
						printf("XXX");
						break;
				}
				break;
			default:
				printf("XXX");
				break;
		}
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
	if (PS2_BUFFER_SIZE - ps2_outbuffer_get_count(1) < 3) {
		//		printf("mouse buffer full, skipping...\n");
		return false;
	}

	uint8_t byte0 =
		((y >> 9) & 1) << 5 |
		((x >> 9) & 1) << 4 |
		1 << 3 |
		b;
	ps2_outbuffer_add(1, byte0);
	ps2_outbuffer_add(1, x);
	ps2_outbuffer_add(1, y);
	return true;
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
