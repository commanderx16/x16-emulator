#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <stdbool.h>
#include <stdint.h>
#include <SDL.h>

bool video_init(uint8_t *chargen);
void video_reset(void);
bool video_step(float mhz);
bool video_update(void);
void video_end(void);

uint8_t video_read(uint8_t reg);
void video_write(uint8_t reg, uint8_t value);

uint8_t via1_read(uint8_t reg);
void via1_write(uint8_t reg, uint8_t value);

#endif
