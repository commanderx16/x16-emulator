#include "video.h"
#include "glue.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

SDL_Window *window;
SDL_Surface *screenSurface;
SDL_Renderer *renderer;
SDL_Texture *sdlTexture;
uint8_t *chargen;

uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT * 4];

bool
video_init(uint8_t *c)
{
	chargen = c;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);

	sdlTexture = SDL_CreateTexture(renderer,
				       SDL_PIXELFORMAT_RGB888,
				       SDL_TEXTUREACCESS_STREAMING,
				       SCREEN_WIDTH, SCREEN_HEIGHT);

	return true;
}

bool
video_update()
{
	for (int y = 0; y < SCREEN_HEIGHT; y++) {
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			uint8_t c = RAM[0x400 + y / 8 * 40 + x / 8];
			int xx = x % 8;
			int yy = y % 8;
			uint8_t s = chargen[c * 8 + yy];
			uint8_t r, g, b;
			if (s & (1 << (7 - xx))) {
				r = 255; g = 255; b = 255;
			} else {
				r = 0; g = 0; b = 0;
			}
			int fbi = (y * SCREEN_WIDTH + x) * 4;
			framebuffer[fbi + 0] = r;
			framebuffer[fbi + 1] = g;
			framebuffer[fbi + 2] = b;
		}
	}

	SDL_UpdateTexture(sdlTexture, NULL, framebuffer, SCREEN_WIDTH * 4);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);
	SDL_RenderPresent(renderer);

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT){
			return false;
		}
		if (event.type == SDL_KEYDOWN){
			return false;
		}
		if (event.type == SDL_MOUSEBUTTONDOWN){
			return false;
		}
	}
	return true;
}

void
video_end()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
