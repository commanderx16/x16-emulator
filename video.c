#include "video.h"

SDL_Window *window;
SDL_Surface *screenSurface;

bool
video_init()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return false;
	}
	window = SDL_CreateWindow("x16emu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (!window) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return false;
	}

	screenSurface = SDL_GetWindowSurface(window);

	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0, 0));
	SDL_UpdateWindowSurface(window);

	return true;
}

bool
video_update()
{
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
  SDL_DestroyWindow(window);
  SDL_Quit();
}
