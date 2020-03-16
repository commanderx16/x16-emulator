// Commander X16 Emulator
// Copyright (c) 2020 Frank van den Hoef
// All rights reserved. License: 2-clause BSD

#pragma once

#include <SDL.h>

void audio_init(const char *dev_name, int num_audio_buffers);
void audio_close(void);
void audio_render(int cpu_clocks);

void audio_usage(void);
