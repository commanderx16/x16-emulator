// Commander X16 Emulator
// Copyright (c) 2020 Frank van den Hoef
// All rights reserved. License: 2-clause BSD

#include "vera_pcm.h"
#include <stdio.h>

static uint8_t  fifo[4096 - 1]; // Actual hardware FIFO is 4kB, but you can only use 4095 bytes.
static unsigned fifo_wridx;
static unsigned fifo_rdidx;
static unsigned fifo_cnt;

static uint8_t ctrl;
static uint8_t rate;

static uint8_t volume_lut[16] = {0, 1, 2, 3, 4, 5, 6, 8, 11, 14, 18, 23, 30, 38, 49, 64};

static int16_t cur_l, cur_r;
static uint8_t phase;

static void
fifo_reset(void)
{
	fifo_wridx = 0;
	fifo_rdidx = 0;
	fifo_cnt   = 0;
}

void
pcm_reset(void)
{
	fifo_reset();
	ctrl  = 0;
	rate  = 0;
	cur_l = 0;
	cur_r = 0;
	phase = 0;
}

void
pcm_write_ctrl(uint8_t val)
{
	if (val & 0x80) {
		fifo_reset();
	}

	ctrl = val & 0x3F;
}

uint8_t
pcm_read_ctrl(void)
{
	uint8_t result = ctrl;
	if (fifo_cnt == sizeof(fifo)) {
		result |= 0x80;
	}
	return result;
}

void
pcm_write_rate(uint8_t val)
{
	rate = val;
}

uint8_t
pcm_read_rate(void)
{
	return rate;
}

void
pcm_write_fifo(uint8_t val)
{
	if (fifo_cnt < sizeof(fifo)) {
		fifo[fifo_wridx++] = val;
		if (fifo_wridx == sizeof(fifo)) {
			fifo_wridx = 0;
		}
		fifo_cnt++;
	}
}

static uint8_t
read_fifo(void)
{
	if (fifo_cnt == 0) {
		return 0;
	}
	uint8_t result = fifo[fifo_rdidx++];
	if (fifo_rdidx == sizeof(fifo)) {
		fifo_rdidx = 0;
	}
	fifo_cnt--;
	return result;
}

bool
pcm_is_fifo_almost_empty(void)
{
	return fifo_cnt < 1024;
}

void
pcm_render(int16_t *buf, unsigned num_samples)
{
	while (num_samples--) {
		uint8_t old_phase = phase;
		phase += rate;
		if ((old_phase & 0x80) != (phase & 0x80)) {
			switch ((ctrl >> 4) & 3) {
				case 0: { // mono 8-bit
					cur_l = (int16_t)read_fifo() << 8;
					cur_r = cur_l;
					break;
				}
				case 1: { // stereo 8-bit
					cur_l = read_fifo() << 8;
					cur_r = read_fifo() << 8;
					break;
				}
				case 2: { // mono 16-bit
					cur_l = read_fifo();
					cur_l |= read_fifo() << 8;
					cur_r = cur_l;
					break;
				}
				case 3: { // stereo 16-bit
					cur_l = read_fifo();
					cur_l |= read_fifo() << 8;
					cur_r = read_fifo();
					cur_r |= read_fifo() << 8;
					break;
				}
			}
		}

		*(buf++) = ((int)cur_l * (int)volume_lut[ctrl & 0xF]) >> 6;
		*(buf++) = ((int)cur_r * (int)volume_lut[ctrl & 0xF]) >> 6;
	}
}
