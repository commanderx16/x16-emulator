// Commander X16 Emulator
// Copyright (c) 2020 Frank van den Hoef
// All rights reserved. License: 2-clause BSD

#pragma once

#include <SDL.h>

void vera_audio_init(SDL_AudioDeviceID dev);
void vera_audio_render(int cpu_clocks);
