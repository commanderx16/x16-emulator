#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <stdbool.h>
#include <SDL.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

bool video_init(void);
bool video_update(void);
void video_end(void);
#endif
