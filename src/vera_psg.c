// Commander X16 Emulator
// Copyright (c) 2020 Frank van den Hoef
// All rights reserved. License: 2-clause BSD

#include "vera_psg.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// enable anti-aliasing for saw and pulse
#define AA_PULSE
#define AA_SAWTOOTH

// aliasing in triangle is already barely audible, so don't bother
// #define AA_TRIANGLE

enum waveform {
	WF_PULSE = 0,
	WF_SAWTOOTH,
	WF_TRIANGLE,
	WF_NOISE,
};

struct channel {
	uint16_t freq;
	uint8_t  volume;
	bool     left, right;
	uint8_t  pw;
	uint8_t  waveform;

	uint8_t  noiseval;
	unsigned phase;

	#if defined(AA_PULSE) || defined(AA_SAWTOOTH) || defined(AA_TRIANGLE)
	unsigned inv_freq;
	#endif

	#ifdef AA_TRIANGLE
	uint16_t freq_3;
	#endif
};

static struct channel channels[16];

static uint8_t volume_lut[64] = {0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 7, 7, 7, 8, 8, 9, 9, 10, 11, 11, 12, 13, 14, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 28, 29, 31, 33, 35, 37, 39, 42, 44, 47, 50, 52, 56, 59, 63};

void
psg_reset(void)
{
	memset(channels, 0, sizeof(channels));
}

static unsigned
calc_inv_freq(const unsigned freq)
{
	static const unsigned n = 0x7FFFFFFF;
	return freq ? n / freq : n;
}

static unsigned
calc_freq_3(const unsigned freq)
{
	return (freq << 1) / 3;
}

static void
set_freq(struct channel *ch, const unsigned freq)
{
	ch->freq = freq;

	#if defined(AA_PULSE) || defined(AA_SAWTOOTH) || defined(AA_TRIANGLE)
	ch->inv_freq = calc_inv_freq(freq);
	#endif

	#ifdef AA_TRIANGLE
	ch->freq_3 = calc_freq_3(freq);
	#endif
}

void
psg_writereg(uint8_t reg, uint8_t val)
{
	reg &= 0x3f;

	int ch  = reg / 4;
	int idx = reg & 3;

	switch (idx) {
		case 0: set_freq(&channels[ch], (channels[ch].freq & 0xFF00) | val); break;
		case 1: set_freq(&channels[ch], (channels[ch].freq & 0x00FF) | (val << 8)); break;
		case 2: {
			channels[ch].right  = (val & 0x80) != 0;
			channels[ch].left   = (val & 0x40) != 0;
			channels[ch].volume = volume_lut[val & 0x3F];
			break;
		}
		case 3: {
			channels[ch].pw       = val & 0x3F;
			channels[ch].waveform = val >> 6;
			break;
		}
	}
}

static unsigned
poly_x(const unsigned phase, const unsigned inv_freq)
{
	return ~((phase * inv_freq) >> 16) & 0x7FFF;
}

static int
poly_blep(unsigned phase, const unsigned inv_freq, const unsigned freq)
{
	const bool dir = phase >= freq;
	if (dir && (phase ^= 0x1FFFF) >= freq) {
		return 0;
	}

	const unsigned x = poly_x(phase, inv_freq);
	const int y1 = (x * x) >> 25, y2 = -y1;

	return dir ? y1 : y2;
}

static int
pulse_blep(unsigned phase, const unsigned inv_freq, const unsigned freq, const unsigned pw)
{
	int y = 0;

	for (int i = 0; i < 2; ++ i) {
		const int x1 = poly_blep(phase, inv_freq, freq), x2 = -x1;
		y += !i ? x1 : x2;

		if (!i) {
			phase = (phase + 0x20000 - ((pw + 1) << 10)) & 0x1FFFF;
		} else {
			break;
		}
	}

	return y;
}

static int
poly_blamp(unsigned phase, const unsigned inv_freq, const unsigned freq)
{
	if (phase >= freq && (phase ^= 0x1FFFF) >= freq) {
		return 0;
	}

	const unsigned x = poly_x(phase, inv_freq);
	const int y = ((x * x) >> 14) * x;

	return y;
}

static int
triangle_blamp(unsigned phase, const unsigned inv_freq, const unsigned freq, const int freq_3)
{
	int y = 0;

	for (int i = 0; i < 2; ++i) {
		const int x1 = poly_blamp(phase, inv_freq, freq), x2 = -x1;
		y += !i ? x1 : x2;

		if (!i) {
			phase = (phase + 0x10000) & 0x1FFFF;
		} else {
			break;
		}
	}

	return ((y >> 16) * freq_3) >> 26;
}

static void
render(int16_t *left, int16_t *right)
{
	int l = 0;
	int r = 0;

	for (int i = 0; i < 16; i++) {
		struct channel *ch = &channels[i];

		unsigned new_phase = (ch->phase + ch->freq) & 0x1FFFF;
		if ((ch->phase & 0x10000) != (new_phase & 0x10000)) {
			ch->noiseval = rand() & 63;
		}
		ch->phase = new_phase;

		int v = 0;
		switch (ch->waveform) {
			case WF_PULSE: {
				v = (ch->phase >> 10) > ch->pw ? 0 : 63;

				#ifdef AA_PULSE
				v += pulse_blep(ch->phase, ch->inv_freq, ch->freq, ch->pw);
				#endif
				break;
			}
			case WF_SAWTOOTH: {
				v = ch->phase >> 11;

				#ifdef AA_SAWTOOTH
				v -= poly_blep(ch->phase, ch->inv_freq, ch->freq);
				#endif
				break;
			}
			case WF_TRIANGLE: {
				v = (ch->phase & 0x10000) ? (~(ch->phase >> 10) & 0x3F) : ((ch->phase >> 10) & 0x3F);

				#ifdef AA_TRIANGLE
				v += triangle_blamp(ch->phase, ch->inv_freq, ch->freq, ch->freq_3);
				#endif
				break;
			}
			case WF_NOISE: v = ch->noiseval; break;
		}
		v -= 32;

		int val = v * (int)ch->volume;

		if (ch->left) {
			l += val;
		}
		if (ch->right) {
			r += val;
		}
	}

	*left  = l;
	*right = r;
}

void
psg_render(int16_t *buf, unsigned num_samples)
{
	while (num_samples--) {
		render(&buf[0], &buf[1]);
		buf += 2;
	}
}
