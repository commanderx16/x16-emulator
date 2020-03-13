// Commander X16 Emulator
// Copyright (c) 2020 Frank van den Hoef
// All rights reserved. License: 2-clause BSD

#include "vera_audio.h"
#include "vera_psg.h"
#include "vera_pcm.h"
#include <stdint.h>
#include <stdio.h>

#define SAMPLES_PER_BLOCK (256)

static SDL_AudioDeviceID audio_dev;

void vera_audio_init(SDL_AudioDeviceID dev) {
    audio_dev = dev;
}

static int vera_clks = 0;
static int cpu_clks  = 0;

void vera_audio_render(int cpu_clocks) {
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

            // Mix PSG and PCM output
            int16_t buf[2 * SAMPLES_PER_BLOCK];
            for (int i = 0; i < 2 * SAMPLES_PER_BLOCK; i++) {
                buf[i] = ((int)psg_buf[i] + (int)pcm_buf[i]) >> 1;
            }

            SDL_QueueAudio(audio_dev, buf, sizeof(buf));
        }
    }
}
