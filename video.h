// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL.h>
#include "glue.h"

bool video_init(int window_scale, char *quality);
void video_reset(void);
bool video_step(float mhz);
bool video_update(void);
void video_end(void);
bool video_get_irq_out(void);
void video_save(SDL_RWops *f);
uint8_t video_read(uint8_t reg, bool debugOn);
void video_write(uint8_t reg, uint8_t value);
void video_update_title(const char* window_title);

uint8_t via1_read(uint8_t reg);
void via1_write(uint8_t reg, uint8_t value);

// For debugging purposes only:
uint8_t video_space_read(uint32_t address);
void video_space_write(uint32_t address, uint8_t value);


#endif
