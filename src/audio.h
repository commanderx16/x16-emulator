// Commander X16 Emulator
// Copyright (c) 2020 Frank van den Hoef
// All rights reserved. License: 2-clause BSD

#pragma once

#include <SDL.h>

#define AUDIO_SAMPLERATE (25000000 / 512)

void audio_init(const char *dev_name, int num_audio_buffers);
void audio_close(void);
void audio_step(int cpu_clocks);
void audio_render();

void audio_usage(void);
