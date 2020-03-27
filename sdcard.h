// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <SDL.h>

extern SDL_RWops *sdcard_file;

void    sdcard_select(bool select);
uint8_t sdcard_handle(uint8_t inbyte);
