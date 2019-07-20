#include "video.h"
#include "glue.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

#define CHARGEN_OFFSET 0x20000
#define PALETTE_OFFSET 0x40200

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *sdlTexture;

static uint8_t video_ram[0x40400];
static uint8_t registers[8];
static uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT * 4];

static const uint8_t palette[] = { // colodore
	0x00, 0x00, 0x00,
	0xff, 0xff, 0xff,
	0x81, 0x33, 0x38,
	0x75, 0xce, 0xc8,
	0x8e, 0x3c, 0x97,
	0x56, 0xac, 0x4d,
	0x2e, 0x2c, 0x9b,
	0xed, 0xf1, 0x71,
	0x8e, 0x50, 0x29,
	0x55, 0x38, 0x00,
	0xc4, 0x6c, 0x71,
	0x4a, 0x4a, 0x4a,
	0x7b, 0x7b, 0x7b,
	0xa9, 0xff, 0x9f,
	0x70, 0x6d, 0xeb,
	0xb2, 0xb2, 0xb2,
};

bool
video_init(uint8_t *chargen)
{
	// init registers
	memset(registers, 0, sizeof(registers));

	// copy chargen into video RAM
	memcpy(&video_ram[CHARGEN_OFFSET], chargen, 4096);

	// init palette
	memcpy(&video_ram[PALETTE_OFFSET], palette, sizeof(palette));

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
			uint32_t addr = 0x400 + (y / 8 * 40 + x / 8) * 2;
			uint8_t ch = video_ram[addr];
			uint8_t col = video_ram[addr + 1];
			int xx = x % 8;
			int yy = y % 8;
			uint8_t s = video_ram[CHARGEN_OFFSET + ch * 8 + yy];
			uint32_t palette_addr;
			if (s & (1 << (7 - xx))) {
				palette_addr = PALETTE_OFFSET + (col & 15) * 3;
			} else {
				palette_addr = PALETTE_OFFSET + (col >> 4) * 3;
			}
			int fbi = (y * SCREEN_WIDTH + x) * 4;
			framebuffer[fbi + 2] = video_ram[palette_addr + 0];
			framebuffer[fbi + 1] = video_ram[palette_addr + 1];
			framebuffer[fbi + 0] = video_ram[palette_addr + 2];
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
