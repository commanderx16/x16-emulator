// Commander X16 Emulator
// Copyright (c) 2020 Frank van den Hoef
// All rights reserved. License: 2-clause BSD

#include "audio.h"
#include "glue.h"
#include "vera_psg.h"
#include "vera_pcm.h"
#include "wav_recorder.h"
#include "ym2151.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
	#define SAMPLES_PER_BUFFER (1024)
	#define SAMP_POS_FRAC_BITS (22)
#else
	#define SAMPLES_PER_BUFFER (256)
	#define SAMP_POS_FRAC_BITS (24)
#endif

#define VERA_SAMP_CLKS_PER_CPU_CLK ((25000000ULL << SAMP_POS_FRAC_BITS) / 512 / MHZ / 1000000)
#define YM_SAMP_CLKS_PER_CPU_CLK ((3579545ULL << SAMP_POS_FRAC_BITS) / 64 / MHZ / 1000000)
#define SAMPLE_BYTES (2 * sizeof(int16_t))
#define SAMP_POS_MASK (SAMPLES_PER_BUFFER - 1)
#define SAMP_POS_MASK_FRAC (((uint32_t)SAMPLES_PER_BUFFER << SAMP_POS_FRAC_BITS) - 1)

// windowed sinc
static const int16_t filter[512] = {
	32767,32765,32761,32755,32746,32736,32723,32707,32690,32670,32649,32625,32598,32570,32539,32507,
	32472,32435,32395,32354,32310,32265,32217,32167,32115,32061,32004,31946,31885,31823,31758,31691,
	31623,31552,31479,31404,31327,31248,31168,31085,31000,30913,30825,30734,30642,30547,30451,30353,
	30253,30151,30048,29943,29835,29726,29616,29503,29389,29273,29156,29037,28916,28793,28669,28544,
	28416,28288,28157,28025,27892,27757,27621,27483,27344,27204,27062,26918,26774,26628,26481,26332,
	26182,26031,25879,25726,25571,25416,25259,25101,24942,24782,24621,24459,24296,24132,23967,23801,
	23634,23466,23298,23129,22959,22788,22616,22444,22271,22097,21923,21748,21572,21396,21219,21042,
	20864,20686,20507,20328,20148,19968,19788,19607,19426,19245,19063,18881,18699,18517,18334,18152,
	17969,17786,17603,17420,17237,17054,16871,16688,16505,16322,16139,15957,15774,15592,15409,15227,
	15046,14864,14683,14502,14321,14141,13961,13781,13602,13423,13245,13067,12890,12713,12536,12360,
	12185,12010,11836,11663,11490,11317,11146,10975,10804,10635,10466,10298,10131, 9964, 9799, 9634,
	 9470, 9306, 9144, 8983, 8822, 8662, 8504, 8346, 8189, 8033, 7878, 7724, 7571, 7419, 7268, 7118,
	 6969, 6822, 6675, 6529, 6385, 6241, 6099, 5958, 5818, 5679, 5541, 5405, 5269, 5135, 5002, 4870,
	 4739, 4610, 4482, 4355, 4229, 4104, 3981, 3859, 3738, 3619, 3500, 3383, 3268, 3153, 3040, 2928,
	 2817, 2708, 2600, 2493, 2388, 2284, 2181, 2079, 1979, 1880, 1783, 1686, 1591, 1498, 1405, 1314,
	 1225, 1136, 1049,  963,  879,  795,  714,  633,  554,  476,  399,  323,  249,  176,  105,   34,
	  -34, -102, -168, -234, -298, -361, -422, -482, -542, -599, -656, -712, -766, -819, -871, -922,
	 -971,-1020,-1067,-1113,-1158,-1202,-1244,-1286,-1326,-1366,-1404,-1441,-1477,-1512,-1546,-1579,
	-1611,-1642,-1671,-1700,-1728,-1755,-1781,-1806,-1830,-1852,-1874,-1896,-1916,-1935,-1953,-1971,
	-1987,-2003,-2018,-2032,-2045,-2058,-2069,-2080,-2090,-2099,-2108,-2116,-2123,-2129,-2134,-2139,
	-2143,-2147,-2150,-2152,-2153,-2154,-2154,-2154,-2153,-2151,-2149,-2146,-2143,-2139,-2135,-2130,
	-2124,-2118,-2112,-2105,-2098,-2090,-2082,-2073,-2064,-2054,-2045,-2034,-2024,-2012,-2001,-1989,
	-1977,-1965,-1952,-1939,-1926,-1912,-1898,-1884,-1870,-1855,-1840,-1825,-1810,-1794,-1778,-1762,
	-1746,-1730,-1714,-1697,-1680,-1663,-1646,-1629,-1612,-1595,-1577,-1560,-1542,-1525,-1507,-1489, 
	-1471,-1453,-1435,-1418,-1400,-1382,-1364,-1346,-1328,-1310,-1292,-1274,-1256,-1238,-1220,-1203,
	-1185,-1167,-1150,-1132,-1115,-1097,-1080,-1063,-1046,-1029,-1012, -995, -978, -962, -945, -929,
     -912, -896, -880, -864, -849, -833, -817, -802, -787, -772, -757, -742, -727, -713, -699, -684,
	 -670, -656, -643, -629, -616, -603, -589, -577, -564, -551, -539, -526, -514, -502, -491, -479,
	 -468, -456, -445, -434, -423, -413, -402, -392, -381, -371, -361, -352, -342, -333, -323, -314,
	 -305, -296, -288, -279, -270, -262, -254, -246, -238, -230, -222, -215, -207, -200, -193, -186,
	 -179, -172, -165, -158, -152, -145, -139, -133, -127, -120, -114, -108, -103,  -97,  -91,  -85,
	  -80,  -74,  -69,  -63,  -58,  -53,  -47,  -42,  -37,  -32,  -27,  -22,  -17,  -12,   -7,   -2
};

static SDL_AudioDeviceID audio_dev;
static int16_t * buffer;
static uint32_t buffer_size = 0;
static uint32_t rdidx = 0;
static uint32_t wridx = 0;
static uint32_t vera_samp_pos_rd = 0;
static uint32_t vera_samp_pos_wr = 0;
static uint32_t vera_samp_pos_hd = 0;
static uint32_t ym_samp_pos_rd = 0;
static uint32_t ym_samp_pos_wr = 0;
static uint32_t ym_samp_pos_hd = 0;
static uint32_t vera_samps_per_host_samps = 0;
static uint32_t ym_samps_per_host_samps = 0;
static uint32_t limiter_amp = 0;

static int16_t psg_buf[2 * SAMPLES_PER_BUFFER];
static int16_t pcm_buf[2 * SAMPLES_PER_BUFFER];
static int16_t ym_buf[2 * SAMPLES_PER_BUFFER];

uint32_t host_sample_rate = 0;

static void
audio_callback(void *userdata, Uint8 *stream, int len)
{
	if (audio_dev == 0) {
		return;
	}

	int expected = SAMPLES_PER_BUFFER * SAMPLE_BYTES;
	if (len != expected) {
		printf("Audio buffer size mismatch! (expected: %d, got: %d)\n", expected, len);
		return;
	}

	uint32_t spos = 0;
	if (rdidx > wridx) {
		uint32_t actual_len = SDL_min(len / SAMPLE_BYTES, (buffer_size - rdidx) / 2);
		if (actual_len > 0) {
			memcpy(&stream[spos], &buffer[rdidx], actual_len * SAMPLE_BYTES);
			spos += actual_len * SAMPLE_BYTES;
			len -= actual_len * SAMPLE_BYTES;
			rdidx = (rdidx + actual_len * 2) % buffer_size;
		}
	}
	uint32_t actual_len = SDL_min(len / SAMPLE_BYTES, (wridx - rdidx) / 2);
	if (actual_len > 0) {
		memcpy(&stream[spos], &buffer[rdidx], actual_len * SAMPLE_BYTES);
		spos += actual_len * SAMPLE_BYTES;
		len -= actual_len * SAMPLE_BYTES;
		rdidx = (rdidx + actual_len * 2) % buffer_size;
	}
	if (len > 0) memset(&stream[spos], 0, len);
}

void
audio_init(const char *dev_name, int num_audio_buffers)
{
	if (audio_dev > 0) {
		audio_close();
	}

	if (dev_name) {
		if (!strcmp("none", dev_name)) {
			return;
		}
	}

	// Set number of buffers
	int num_bufs = num_audio_buffers;
	if (num_bufs < 3) {
		num_bufs = 3;
	}
	if (num_bufs > 1024) {
		num_bufs = 1024;
	}
	buffer_size = SAMPLES_PER_BUFFER * num_bufs * 2;

	// Allocate audio buffer
	buffer = malloc(buffer_size * sizeof(int16_t));
	rdidx = 0;
	wridx = 0;

	SDL_AudioSpec desired;
	SDL_AudioSpec obtained;

	// Setup SDL audio
	memset(&desired, 0, sizeof(desired));
	desired.freq     = AUDIO_SAMPLERATE;
	desired.format   = AUDIO_S16SYS;
	desired.samples  = SAMPLES_PER_BUFFER;
	desired.channels = 2;
	desired.callback = audio_callback;

	audio_dev = SDL_OpenAudioDevice(dev_name, 0, &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	if (audio_dev <= 0) {
		fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
		if (dev_name != NULL) {
			audio_usage();
		}
		exit(-1);
	}
	if (obtained.freq <= 0 || (AUDIO_SAMPLERATE / obtained.freq) > SAMPLES_PER_BUFFER) {
		fprintf(stderr, "Obtained sample rate is too low");
		audio_close();
		return;
	}

	// Init YM2151 emulation. 3.579545 MHz clock
	YM_Create(3579545);
	YM_init(3579545/64, 60);

	host_sample_rate = obtained.freq;
	vera_samps_per_host_samps = ((25000000ULL << SAMP_POS_FRAC_BITS) / 512 / host_sample_rate);
	ym_samps_per_host_samps = ((3579545ULL << SAMP_POS_FRAC_BITS) / 64 / host_sample_rate);
	vera_samp_pos_rd = 0;
	vera_samp_pos_wr = 0;
	vera_samp_pos_hd = 0;
	ym_samp_pos_rd = 0;
	ym_samp_pos_wr = 0;
	ym_samp_pos_hd = 0;
	limiter_amp = (1 << 16);

	psg_buf[0] = psg_buf[1] = 0;
	pcm_buf[0] = pcm_buf[1] = 0;
	ym_buf[0] = ym_buf[1] = 0;

	// Start playback
	SDL_PauseAudioDevice(audio_dev, 0);
}

void
audio_close(void)
{
	if (audio_dev == 0) {
		return;
	}

	SDL_CloseAudioDevice(audio_dev);
	audio_dev = 0;

	// Free audio buffers
	if (buffer != NULL) {
		free(buffer);
		buffer = NULL;
	}
}

void
audio_step(int cpu_clocks)
{
	// Accumulate how many samples each source have to render
	if (audio_dev == 0) {
		return;
	}

	while (cpu_clocks > 0) {
		// Only the source with the higest sample rate (YM2151) is needed for calculation
		uint32_t max_cpu_clks_ym = ((ym_samp_pos_rd - ym_samp_pos_hd - (1 << SAMP_POS_FRAC_BITS)) & SAMP_POS_MASK_FRAC) / YM_SAMP_CLKS_PER_CPU_CLK;
		uint32_t max_cpu_clks = SDL_min(cpu_clocks, max_cpu_clks_ym);
		vera_samp_pos_hd = (vera_samp_pos_hd + max_cpu_clks * VERA_SAMP_CLKS_PER_CPU_CLK) & SAMP_POS_MASK_FRAC;
		ym_samp_pos_hd = (ym_samp_pos_hd + max_cpu_clks * YM_SAMP_CLKS_PER_CPU_CLK) & SAMP_POS_MASK_FRAC;
		cpu_clocks -= max_cpu_clks;
		if (cpu_clocks > 0) audio_render();
	}
}

void
audio_render()
{
	// Render all audio sources until read and write positions catch up
	// This happens when there's a change to sound registers or one of the
	// sources' sample buffer head position is too far
	if (audio_dev == 0) {
		return;
	}

	uint32_t pos, len;

	pos = (vera_samp_pos_wr + 1) & SAMP_POS_MASK;
	len = ((vera_samp_pos_hd >> SAMP_POS_FRAC_BITS) - vera_samp_pos_wr) & SAMP_POS_MASK;
	vera_samp_pos_wr = vera_samp_pos_hd >> SAMP_POS_FRAC_BITS;
	if (pos + len > SAMPLES_PER_BUFFER) {
		psg_render(&psg_buf[pos * 2], SAMPLES_PER_BUFFER - pos);
		pcm_render(&pcm_buf[pos * 2], SAMPLES_PER_BUFFER - pos);
		len -= SAMPLES_PER_BUFFER - pos;
		pos = 0;
	}
	if (len > 0) {
		psg_render(&psg_buf[pos * 2], len);
		pcm_render(&pcm_buf[pos * 2], len);
	}

	pos = (ym_samp_pos_wr + 1) & SAMP_POS_MASK;
	len = ((ym_samp_pos_hd >> SAMP_POS_FRAC_BITS) - ym_samp_pos_wr) & SAMP_POS_MASK;
	ym_samp_pos_wr = ym_samp_pos_hd >> SAMP_POS_FRAC_BITS;
	if ((pos + len) > SAMPLES_PER_BUFFER) {
		YM_stream_update((uint16_t *)&ym_buf[pos * 2], SAMPLES_PER_BUFFER - pos);
		len -= SAMPLES_PER_BUFFER - pos;
		pos = 0;
	}
	if (len > 0) {
		YM_stream_update((uint16_t *)&ym_buf[pos * 2], len);
	}

	uint32_t wridx_old = wridx;
	uint32_t len_vera = (vera_samp_pos_hd - vera_samp_pos_rd) & SAMP_POS_MASK_FRAC;
	uint32_t len_ym = (ym_samp_pos_hd - ym_samp_pos_rd) & SAMP_POS_MASK_FRAC;
	if (len_vera < (4 << SAMP_POS_FRAC_BITS) || len_ym < (4 << SAMP_POS_FRAC_BITS)) {
		// not enough samples yet, at least 4 are needed for the filter
		return;
	}
	len_vera = (len_vera - (4 << SAMP_POS_FRAC_BITS)) / vera_samps_per_host_samps;
	len_ym = (len_ym - (4 << SAMP_POS_FRAC_BITS)) / ym_samps_per_host_samps;
	len = SDL_min(len_vera, len_ym);
	SDL_LockAudioDevice(audio_dev);
	for (int i = 0; i < len; i++) {
		int32_t samp[8];
		int32_t filter_idx = 0;
		int32_t vera_out_l = 0;
		int32_t vera_out_r = 0;
		int32_t ym_out_l = 0;
		int32_t ym_out_r = 0;
		// Don't resample VERA outputs if the host sample rate is as desired
		if (host_sample_rate == AUDIO_SAMPLERATE) {
			pos = (vera_samp_pos_rd >> SAMP_POS_FRAC_BITS) * 2;
			vera_out_l = ((int32_t)psg_buf[pos] + (int32_t)pcm_buf[pos]) / 2 * 32768;
			vera_out_r = ((int32_t)psg_buf[pos + 1] + (int32_t)pcm_buf[pos + 1]) / 2 * 32768;
		} else {
			filter_idx = (vera_samp_pos_rd >> (SAMP_POS_FRAC_BITS - 8)) & 0xff;
			for (int j = 0; j < 8; j += 2) {
				pos = (vera_samp_pos_rd >> SAMP_POS_FRAC_BITS) * 2;
				samp[j] = ((int32_t)psg_buf[pos] + (int32_t)pcm_buf[pos]) / 2;
				samp[j + 1] = ((int32_t)psg_buf[pos + 1] + (int32_t)pcm_buf[pos + 1]) / 2;
				pos = (pos + 2) & (SAMP_POS_MASK * 2);
			}
			vera_out_l += samp[0] * filter[256 + filter_idx];
			vera_out_r += samp[1] * filter[256 + filter_idx];
			vera_out_l += samp[2] * filter[  0 + filter_idx];
			vera_out_r += samp[3] * filter[  0 + filter_idx];
			vera_out_l += samp[4] * filter[255 - filter_idx];
			vera_out_r += samp[5] * filter[255 - filter_idx];
			vera_out_l += samp[6] * filter[511 - filter_idx];
			vera_out_r += samp[7] * filter[511 - filter_idx];
		}
		filter_idx = (ym_samp_pos_rd >> (SAMP_POS_FRAC_BITS - 8)) & 0xff;
		for (int j = 0; j < 8; j += 2) {
			pos = (ym_samp_pos_rd >> SAMP_POS_FRAC_BITS) * 2;
			samp[j] = ym_buf[pos];
			samp[j + 1] = ym_buf[pos + 1];
			pos = (pos + 2) & (SAMP_POS_MASK * 2);
		}
		ym_out_l += samp[0] * filter[256 + filter_idx];
		ym_out_r += samp[1] * filter[256 + filter_idx];
		ym_out_l += samp[2] * filter[  0 + filter_idx];
		ym_out_r += samp[3] * filter[  0 + filter_idx];
		ym_out_l += samp[4] * filter[255 - filter_idx];
		ym_out_r += samp[5] * filter[255 - filter_idx];
		ym_out_l += samp[6] * filter[511 - filter_idx];
		ym_out_r += samp[7] * filter[511 - filter_idx];
		// Mixing is according to Proto3 hardware recording
		// Loudest single PSG channel is 1/8 times the max output
		// mix = (psg + pcm) * 2 + ym / 2
		int32_t mix_l = (vera_out_l >> 13) + (ym_out_l >> 16);
		int32_t mix_r = (vera_out_r >> 13) + (ym_out_r >> 16);
		uint32_t amp = SDL_max(SDL_abs(mix_l), SDL_abs(mix_r));
		if (amp > 32767) {
			uint32_t limiter_amp_new = (32767 << 16) / amp;
			limiter_amp = SDL_min(limiter_amp_new, limiter_amp);
		}
		buffer[wridx++] = (int16_t)((mix_l * limiter_amp) >> 16);
		buffer[wridx++] = (int16_t)((mix_r * limiter_amp) >> 16);
		if (limiter_amp < (1 << 16)) limiter_amp++;
		vera_samp_pos_rd = (vera_samp_pos_rd + vera_samps_per_host_samps) & SAMP_POS_MASK_FRAC;
		ym_samp_pos_rd = (ym_samp_pos_rd + ym_samps_per_host_samps) & SAMP_POS_MASK_FRAC;
		if (wridx == buffer_size) {
			wav_recorder_process(&buffer[wridx_old], (buffer_size - wridx_old) / 2);
			wridx = 0;
			wridx_old = 0;
		}
	}
	if ((wridx - wridx_old) > 0) {
		wav_recorder_process(&buffer[wridx_old], (wridx - wridx_old) / 2);
	}
	SDL_UnlockAudioDevice(audio_dev);

	// catch up all buffers if they are too far behind
	uint32_t skip = len_vera - len;
	if (skip > 1) {
		vera_samp_pos_rd = (vera_samp_pos_rd + vera_samps_per_host_samps) & SAMP_POS_MASK_FRAC;
	}
	skip = len_ym - len;
	if (skip > 1) {
		ym_samp_pos_rd = (ym_samp_pos_rd + ym_samps_per_host_samps) & SAMP_POS_MASK_FRAC;
	}
}

void
audio_usage(void)
{
	// SDL_GetAudioDeviceName doesn't work if audio isn't initialized.
	// Since argument parsing happens before initializing SDL, ensure the
	// audio subsystem is initialized before printing audio device names.
	SDL_InitSubSystem(SDL_INIT_AUDIO);

	// List all available sound devices
	printf("The following sound output devices are available:\n");
	const int sounds = SDL_GetNumAudioDevices(0);
	for (int i = 0; i < sounds; ++i) {
		printf("\t%s\n", SDL_GetAudioDeviceName(i, 0));
	}

	SDL_Quit();
	exit(1);
}
