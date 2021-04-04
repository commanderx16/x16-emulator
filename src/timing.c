#ifndef __APPLE__
#define _XOPEN_SOURCE   600
#define _POSIX_C_SOURCE 1
#endif
#include "glue.h"
#include "video.h"
#include <SDL.h>
#include <stdio.h>
#include <unistd.h>

int frames;
int32_t sdlTicks_base;
int32_t last_perf_update;
int32_t perf_frame_count;
char window_title[30];

void
timing_init() {
	frames = 0;
	sdlTicks_base = SDL_GetTicks();
	last_perf_update = 0;
	perf_frame_count = 0;
}

void
timing_update()
{
	frames++;
	int32_t sdlTicks = SDL_GetTicks() - sdlTicks_base;
	int32_t diff_time = 1000 * frames / 60 - sdlTicks;
	if (!warp_mode && diff_time > 0) {
		usleep(1000 * diff_time);
	}

	if (sdlTicks - last_perf_update > 5000) {
		int32_t frameCount = frames - perf_frame_count;
		int perf = frameCount / 3;

		if (perf < 100 || warp_mode) {
			sprintf(window_title, "Commander X16 (%d%%)", perf);
			video_update_title(window_title);
		} else {
			video_update_title("Commander X16");
		}

		perf_frame_count = frames;
		last_perf_update = sdlTicks;
	}

	if (log_speed) {
		float frames_behind = -((float)diff_time / 16.666666);
		int load = (int)((1 + frames_behind) * 100);
		printf("Load: %d%%\n", load > 100 ? 100 : load);

		if ((int)frames_behind > 0) {
			printf("Rendering is behind %d frames.\n", -(int)frames_behind);
		} else {
		}
	}
}

