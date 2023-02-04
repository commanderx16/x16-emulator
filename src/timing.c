#ifndef __APPLE__
#define _XOPEN_SOURCE   600
#define _POSIX_C_SOURCE 1
#endif
#include "glue.h"
#include "video.h"
#include "cpu/fake6502.h"
#include <SDL.h>
#include <stdio.h>
#include <unistd.h>

uint32_t frames;
uint32_t sdlTicks_base;
uint32_t last_perf_update;
uint32_t clockticks6502_old;
int64_t cpu_ticks;
int64_t last_perf_cpu_ticks;
char window_title[30];

void
timing_init() {
	frames = 0;
	sdlTicks_base = SDL_GetTicks();
	last_perf_update = 0;
	last_perf_cpu_ticks = 0;
	clockticks6502_old = clockticks6502;
	cpu_ticks = 0;
}

void
timing_update()
{
	frames++;
	cpu_ticks += clockticks6502 - clockticks6502_old;
	clockticks6502_old = clockticks6502;
	uint32_t sdlTicks = SDL_GetTicks() - sdlTicks_base;
	int64_t diff_time = cpu_ticks / MHZ - sdlTicks * 1000LL;
	if (!warp_mode && diff_time > 0) {
		if (diff_time >= 1000000) {
			sleep(diff_time / 1000000);
			diff_time %= 1000000;
		}
		usleep(diff_time);
	}

	if (sdlTicks - last_perf_update > 5000) {
		uint32_t perf = (cpu_ticks - last_perf_cpu_ticks) / (MHZ * 50000);

		if (perf < 100 || warp_mode) {
			sprintf(window_title, "Commander X16 (%d%%)", perf);
			video_update_title(window_title);
		} else {
			video_update_title("Commander X16");
		}

		last_perf_cpu_ticks = cpu_ticks;
		last_perf_update = sdlTicks;
	}

	if (log_speed) {
		float frames_behind = -((float)diff_time * 6e-5);
		int load = (int)((1 + frames_behind) * 100);
		printf("Load: %d%%\n", load > 100 ? 100 : load);

		if ((int)frames_behind > 0) {
			printf("Rendering is behind %d frames.\n", -(int)frames_behind);
		} else {
		}
	}
}

