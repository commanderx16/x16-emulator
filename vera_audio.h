#pragma once

#include <SDL.h>

void vera_audio_init(SDL_AudioDeviceID dev);
void vera_audio_render(int cpu_clocks);
