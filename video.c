#include "video.h"
#include "glue.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

#define CHARGEN_OFFSET 0x20000

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *sdlTexture;

static uint8_t video_ram[0x40400];
static uint8_t registers[8];
static uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT * 4];

bool
video_init(uint8_t *chargen)
{
	memcpy(&video_ram[CHARGEN_OFFSET], chargen, 4096);

	memset(registers, 0, sizeof(registers));

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
			uint8_t c = video_ram[0x400 + y / 8 * 40 + x / 8];
			int xx = x % 8;
			int yy = y % 8;
			uint8_t s = video_ram[CHARGEN_OFFSET + c * 8 + yy];
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

//uint32_t address = 0;

uint32_t
get_and_inc_address()
{
	uint32_t address = ((uint32_t)(registers[0] & 7)) << 16 | registers[1] << 8 | registers[2];
	uint32_t new_address = address + (registers[0] >> 4);
	registers[0] = (registers[0] & 0xf0) | ((new_address >> 16) & 0x0f);
	registers[1] = (new_address >> 8) & 0xff;
	registers[2] = new_address & 0xff;
//	printf("address = %06x, new = %06x\n", address, new_address);
	return address;
}

uint8_t
video_read(uint8_t reg)
{
	if (reg == 3) {
		return video_ram[get_and_inc_address()];
	} else {
		return registers[reg];
	}
}

void
video_write(uint8_t reg, uint8_t value)
{
//	printf("registers[%d] = $%02x\n", reg, value);
	if (reg == 3) {
		video_ram[get_and_inc_address()] = value;
	} else {
		registers[reg] = value;
	}
}
