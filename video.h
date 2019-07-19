#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <stdbool.h>
#include <SDL.h>

bool video_init(uint8_t *chargen);
bool video_update(void);
void video_end(void);
#endif
