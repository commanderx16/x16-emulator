// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <stdbool.h>
#include <stdint.h>
#include <SDL.h>
#include "glue.h"

#ifdef VERA_V0_8
bool video_init(int window_scale);
#else
bool video_init(uint8_t *chargen, int window_scale);
#endif
void video_reset(void);
bool video_step(float mhz);
bool video_update(void);
void video_end(void);
bool video_get_irq_out(void);

uint8_t video_read(uint8_t reg);
void video_write(uint8_t reg, uint8_t value);

uint8_t via1_read(uint8_t reg);
void via1_write(uint8_t reg, uint8_t value);

#endif
