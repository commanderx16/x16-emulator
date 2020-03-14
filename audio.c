// Commander X16 Emulator
// Copyright (c) 2020 Frank van den Hoef
// All rights reserved. License: 2-clause BSD

#include "audio.h"
#include "vera_psg.h"
#include "vera_pcm.h"
#include "ym2151.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SAMPLERATE (25000000 / 512)
#define SAMPLES_PER_BLOCK (256)

static SDL_AudioDeviceID audio_dev;
static int               vera_clks = 0;
static int               cpu_clks  = 0;

void
audio_init(const char *dev_name)
{
	SDL_AudioSpec desired;
	SDL_AudioSpec obtained;

	// Setup SDL audio
	memset(&desired, 0, sizeof(desired));
	desired.freq     = SAMPLERATE;
	desired.format   = AUDIO_S16SYS;
	desired.channels = 2;

	if (audio_dev > 0) {
		SDL_CloseAudioDevice(audio_dev);
	}

	audio_dev = SDL_OpenAudioDevice(dev_name, 0, &desired, &obtained, 9 /* freq | samples */);
	if (audio_dev <= 0) {
		fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
		if (dev_name != NULL) {
			audio_usage();
		}
		exit(-1);
	}

	// Init YM2151 emulation. 4 MHz clock
	YM_Create(4000000);
	YM_init(obtained.freq, 60);

	// Start playback
	SDL_PauseAudioDevice(audio_dev, 0);
}

void
audio_close(void)
{
	SDL_CloseAudioDevice(audio_dev);
}

void
audio_render(int cpu_clocks)
{
	cpu_clks += cpu_clocks;
	if (cpu_clks > 8) {
		int c = cpu_clks / 8;
		cpu_clks -= c * 8;
		vera_clks += c * 25;
	}

	while (vera_clks >= 512 * SAMPLES_PER_BLOCK) {
		vera_clks -= 512 * SAMPLES_PER_BLOCK;

		if (audio_dev != 0) {
			int16_t psg_buf[2 * SAMPLES_PER_BLOCK];
			psg_render(psg_buf, SAMPLES_PER_BLOCK);

			int16_t pcm_buf[2 * SAMPLES_PER_BLOCK];
			pcm_render(pcm_buf, SAMPLES_PER_BLOCK);

			int16_t ym_buf[2 * SAMPLES_PER_BLOCK];
			YM_stream_update((uint16_t *)ym_buf, SAMPLES_PER_BLOCK);

			// Mix PSG, PCM and YM output
			int16_t buf[2 * SAMPLES_PER_BLOCK];
			for (int i = 0; i < 2 * SAMPLES_PER_BLOCK; i++) {
				buf[i] = ((int)psg_buf[i] + (int)pcm_buf[i] + (int)ym_buf[i]) / 3;
			}

			SDL_QueueAudio(audio_dev, buf, sizeof(buf));
		}
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
