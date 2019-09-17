// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include "video.h"
#include "memory.h"
#include "ps2.h"
#include "glue.h"
#include "debugger.h"
#include "gif.h"
#include "vera_spi.h"

#ifdef VERA_V0_8
#define ADDR_VRAM_START     0x00000
#define ADDR_VRAM_END       0x20000

#define ADDR_COMPOSER_START 0xF0000
#define ADDR_COMPOSER_END   0xF1000
#define ADDR_PALETTE_START  0xF1000
#define ADDR_PALETTE_END    0xF2000
#define ADDR_LAYER1_START   0xF2000
#define ADDR_LAYER1_END     0xF3000
#define ADDR_LAYER2_START   0xF3000
#define ADDR_LAYER2_END     0xF4000
#define ADDR_SPRITES_START  0xF4000
#define ADDR_SPRITES_END    0xF5000
#define ADDR_SPRDATA_START  0xF5000
#define ADDR_SPRDATA_END    0xF6000
#else
#define ADDR_VRAM_START     0x00000
#define ADDR_VRAM_END       0x20000
#define ADDR_CHARSET_START  0x20000
#define ADDR_CHARSET_END    0x21000
#define ADDR_LAYER1_START   0x40000
#define ADDR_LAYER1_END     0x40010
#define ADDR_LAYER2_START   0x40010
#define ADDR_LAYER2_END     0x40020
#define ADDR_SPRITES_START  0x40020
#define ADDR_SPRITES_END    0x40040
#define ADDR_COMPOSER_START 0x40040
#define ADDR_COMPOSER_END   0x40060
#define ADDR_PALETTE_START  0x40200
#define ADDR_PALETTE_END    0x40400
#define ADDR_SPRDATA_START  0x40800
#define ADDR_SPRDATA_END    0x41000
#endif
#define ADDR_SPI_START      0xF7000
#define ADDR_SPI_END        0xF8000

#define ESC_IS_BREAK /* if enabled, Esc sends Break/Pause key instead of Esc */

//#define NUM_SPRITES 128
#define NUM_SPRITES 16 /* XXX speedup */

// both VGA and NTSC
#define SCAN_WIDTH 800
#define SCAN_HEIGHT 525

// VGA
#define VGA_FRONT_PORCH_X 16
#define VGA_FRONT_PORCH_Y 10
#define VGA_PIXEL_FREQ 25.175

// NTSC: 262.5 lines per frame, lower field first
#define NTSC_FRONT_PORCH_X 80
#define NTSC_FRONT_PORCH_Y 22
#define NTSC_PIXEL_FREQ (15.750 * 800 / 1000)
#define TITLE_SAFE_X 0.067
#define TITLE_SAFE_Y 0.05

// visible area we're darwing
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define SCREEN_RAM_OFFSET 0x00000

#ifdef __APPLE__
#define LSHORTCUT_KEY SDL_SCANCODE_LGUI
#define RSHORTCUT_KEY SDL_SCANCODE_RGUI
#else
#define LSHORTCUT_KEY SDL_SCANCODE_LCTRL
#define RSHORTCUT_KEY SDL_SCANCODE_RCTRL
#endif

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *sdlTexture;
static bool is_fullscreen = false;

static uint8_t video_ram[0x20000];
#ifndef VERA_V0_8
static uint8_t chargen_rom[4096];
#endif
static uint8_t palette[256 * 2];
static uint8_t sprite_data[256][8];

// I/O registers
static uint32_t io_addr[2];
static uint8_t io_inc[2];
bool io_addrsel;

static uint8_t ien = 0;
static uint8_t isr = 0;

static uint8_t reg_layer[2][16];
static uint8_t reg_sprites[16];
static uint8_t reg_composer[32];

static uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT * 4];

static GifWriter gif_writer;

static const uint16_t default_palette[] = {
0x0000,0xfff,0x800,0xafe,0xc4c,0x0c5,0x00a,0xee7,0xd85,0x640,0xf77,0x333,0x777,0xaf6,0x08f,0xbbb,0x000,0x111,0x222,0x333,0x444,0x555,0x666,0x777,0x888,0x999,0xaaa,0xbbb,0xccc,0xddd,0xeee,0xfff,0x211,0x433,0x644,0x866,0xa88,0xc99,0xfbb,0x211,0x422,0x633,0x844,0xa55,0xc66,0xf77,0x200,0x411,0x611,0x822,0xa22,0xc33,0xf33,0x200,0x400,0x600,0x800,0xa00,0xc00,0xf00,0x221,0x443,0x664,0x886,0xaa8,0xcc9,0xfeb,0x211,0x432,0x653,0x874,0xa95,0xcb6,0xfd7,0x210,0x431,0x651,0x862,0xa82,0xca3,0xfc3,0x210,0x430,0x640,0x860,0xa80,0xc90,0xfb0,0x121,0x343,0x564,0x786,0x9a8,0xbc9,0xdfb,0x121,0x342,0x463,0x684,0x8a5,0x9c6,0xbf7,0x120,0x241,0x461,0x582,0x6a2,0x8c3,0x9f3,0x120,0x240,0x360,0x480,0x5a0,0x6c0,0x7f0,0x121,0x343,0x465,0x686,0x8a8,0x9ca,0xbfc,0x121,0x242,0x364,0x485,0x5a6,0x6c8,0x7f9,0x020,0x141,0x162,0x283,0x2a4,0x3c5,0x3f6,0x020,0x041,0x061,0x082,0x0a2,0x0c3,0x0f3,0x122,0x344,0x466,0x688,0x8aa,0x9cc,0xbff,0x122,0x244,0x366,0x488,0x5aa,0x6cc,0x7ff,0x022,0x144,0x166,0x288,0x2aa,0x3cc,0x3ff,0x022,0x044,0x066,0x088,0x0aa,0x0cc,0x0ff,0x112,0x334,0x456,0x668,0x88a,0x9ac,0xbcf,0x112,0x224,0x346,0x458,0x56a,0x68c,0x79f,0x002,0x114,0x126,0x238,0x24a,0x35c,0x36f,0x002,0x014,0x016,0x028,0x02a,0x03c,0x03f,0x112,0x334,0x546,0x768,0x98a,0xb9c,0xdbf,0x112,0x324,0x436,0x648,0x85a,0x96c,0xb7f,0x102,0x214,0x416,0x528,0x62a,0x83c,0x93f,0x102,0x204,0x306,0x408,0x50a,0x60c,0x70f,0x212,0x434,0x646,0x868,0xa8a,0xc9c,0xfbe,0x211,0x423,0x635,0x847,0xa59,0xc6b,0xf7d,0x201,0x413,0x615,0x826,0xa28,0xc3a,0xf3c,0x201,0x403,0x604,0x806,0xa08,0xc09,0xf0b
};

static uint8_t video_space_read(uint32_t address);

void
video_reset()
{
	// init I/O registers
	memset(io_addr, 0, sizeof(io_addr));
	memset(io_inc, 0, sizeof(io_inc));
	io_addrsel = 0;

	// init Layer registers
	memset(reg_layer, 0, sizeof(reg_layer));
#ifndef VERA_V0_8
	uint32_t tile_base = ADDR_CHARSET_START; // uppercase PETSCII
	reg_layer[0][0] = 1; // mode=0, enabled=1
	reg_layer[0][4] = tile_base >> 2;
	reg_layer[0][5] = tile_base >> 10;
#endif

	// init sprite registers
	memset(reg_sprites, 0, sizeof(reg_sprites));

	// init composer registers
	memset(reg_composer, 0, sizeof(reg_composer));
	reg_composer[1] = 128; // hscale = 1.0
	reg_composer[2] = 128; // vscale = 1.0

	// init sprite data
	memset(sprite_data, 0, sizeof(sprite_data));

	// copy palette
	memcpy(palette, default_palette, sizeof(palette));
	for (int i = 0; i < 256; i++) {
		palette[i * 2 + 0] = default_palette[i] & 0xff;
		palette[i * 2 + 1] = default_palette[i] >> 8;
	}
}

bool
#ifdef VERA_V0_8
video_init(int window_scale)
#else
video_init(uint8_t *in_chargen, int window_scale)
#endif
{
#ifndef VERA_V0_8
	// copy chargen
	memcpy(chargen_rom, in_chargen, sizeof(chargen_rom));
#endif

	video_reset();

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(SCREEN_WIDTH * window_scale, SCREEN_HEIGHT * window_scale, 0, &window, &renderer);
#ifndef __MORPHOS__
	SDL_SetWindowResizable(window, true);
#endif
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

	sdlTexture = SDL_CreateTexture(renderer,
				       SDL_PIXELFORMAT_RGB888,
				       SDL_TEXTUREACCESS_STREAMING,
				       SCREEN_WIDTH, SCREEN_HEIGHT);

	if (record_gif) {
		record_gif = GifBegin(&gif_writer, gif_path, SCREEN_WIDTH, SCREEN_HEIGHT, 1, 8, false);
	}

	return true;
}

#define EXTENDED_FLAG 0x100

int
ps2_scancode_from_SDLKey(SDL_Scancode k)
{
	switch (k) {
		case SDL_SCANCODE_GRAVE:
			return 0x0e;
		case SDL_SCANCODE_BACKSPACE:
			return 0x66;
		case SDL_SCANCODE_TAB:
			return 0xd;
		case SDL_SCANCODE_CLEAR:
			return 0;
		case SDL_SCANCODE_RETURN:
			return 0x5a;
		case SDL_SCANCODE_PAUSE:
			return 0;
		case SDL_SCANCODE_ESCAPE:
#ifdef ESC_IS_BREAK
			return 0xff;
#else
			return 0x76;
#endif
		case SDL_SCANCODE_SPACE:
			return 0x29;
		case SDL_SCANCODE_APOSTROPHE:
			return 0x52;
		case SDL_SCANCODE_COMMA:
			return 0x41;
		case SDL_SCANCODE_MINUS:
			return 0x4e;
		case SDL_SCANCODE_PERIOD:
			return 0x49;
		case SDL_SCANCODE_SLASH:
			return 0x4a;
		case SDL_SCANCODE_0:
			return 0x45;
		case SDL_SCANCODE_1:
			return 0x16;
		case SDL_SCANCODE_2:
			return 0x1e;
		case SDL_SCANCODE_3:
			return 0x26;
		case SDL_SCANCODE_4:
			return 0x25;
		case SDL_SCANCODE_5:
			return 0x2e;
		case SDL_SCANCODE_6:
			return 0x36;
		case SDL_SCANCODE_7:
			return 0x3d;
		case SDL_SCANCODE_8:
			return 0x3e;
		case SDL_SCANCODE_9:
			return 0x46;
		case SDL_SCANCODE_SEMICOLON:
			return 0x4c;
		case SDL_SCANCODE_EQUALS:
			return 0x55;
		case SDL_SCANCODE_LEFTBRACKET:
			return 0x54;
		case SDL_SCANCODE_BACKSLASH:
			return 0x5d;
		case SDL_SCANCODE_RIGHTBRACKET:
			return 0x5b;
		case SDL_SCANCODE_A:
			return 0x1c;
		case SDL_SCANCODE_B:
			return 0x32;
		case SDL_SCANCODE_C:
			return 0x21;
		case SDL_SCANCODE_D:
			return 0x23;
		case SDL_SCANCODE_E:
			return 0x24;
		case SDL_SCANCODE_F:
			return 0x2b;
		case SDL_SCANCODE_G:
			return 0x34;
		case SDL_SCANCODE_H:
			return 0x33;
		case SDL_SCANCODE_I:
			return 0x43;
		case SDL_SCANCODE_J:
			return 0x3B;
		case SDL_SCANCODE_K:
			return 0x42;
		case SDL_SCANCODE_L:
			return 0x4B;
		case SDL_SCANCODE_M:
			return 0x3A;
		case SDL_SCANCODE_N:
			return 0x31;
		case SDL_SCANCODE_O:
			return 0x44;
		case SDL_SCANCODE_P:
			return 0x4D;
		case SDL_SCANCODE_Q:
			return 0x15;
		case SDL_SCANCODE_R:
			return 0x2D;
		case SDL_SCANCODE_S:
			return 0x1B;
		case SDL_SCANCODE_T:
			return 0x2C;
		case SDL_SCANCODE_U:
			return 0x3C;
		case SDL_SCANCODE_V:
			return 0x2A;
		case SDL_SCANCODE_W:
			return 0x1D;
		case SDL_SCANCODE_X:
			return 0x22;
		case SDL_SCANCODE_Y:
			return 0x35;
		case SDL_SCANCODE_Z:
			return 0x1A;
		case SDL_SCANCODE_DELETE:
			return 0;
		case SDL_SCANCODE_UP:
			return 0x75 | EXTENDED_FLAG;
		case SDL_SCANCODE_DOWN:
			return 0x72 | EXTENDED_FLAG;
		case SDL_SCANCODE_RIGHT:
			return 0x74 | EXTENDED_FLAG;
		case SDL_SCANCODE_LEFT:
			return 0x6b | EXTENDED_FLAG;
		case SDL_SCANCODE_INSERT:
			return 0;
		case SDL_SCANCODE_HOME:
			return 0x6c | EXTENDED_FLAG;
		case SDL_SCANCODE_END:
			return 0;
		case SDL_SCANCODE_PAGEUP:
			return 0;
		case SDL_SCANCODE_PAGEDOWN:
			return 0;
		case SDL_SCANCODE_F1:
			return 0x05;
		case SDL_SCANCODE_F2:
			return 0x06;
		case SDL_SCANCODE_F3:
			return 0x04;
		case SDL_SCANCODE_F4:
			return 0x0c;
		case SDL_SCANCODE_F5:
			return 0x03;
		case SDL_SCANCODE_F6:
			return 0x0b;
		case SDL_SCANCODE_F7:
			return 0x83;
		case SDL_SCANCODE_F8:
			return 0x0a;
		case SDL_SCANCODE_F9:
			return 0x01;
		case SDL_SCANCODE_F10:
			return 0x09;
		case SDL_SCANCODE_F11:
			return 0x78;
		case SDL_SCANCODE_F12:
			return 0x07;
		case SDL_SCANCODE_RSHIFT:
			return 0x59;
		case SDL_SCANCODE_LSHIFT:
			return 0x12;
		case SDL_SCANCODE_LCTRL:
			return 0x14;
		case SDL_SCANCODE_RCTRL:
			return 0x14 | EXTENDED_FLAG;
		case SDL_SCANCODE_LALT:
			return 0x11;
		case SDL_SCANCODE_RALT:
			return 0x11 | EXTENDED_FLAG;
//		case SDL_SCANCODE_LGUI: // Windows/Command
//			return 0x5b | EXTENDED_FLAG;
		case SDL_SCANCODE_NONUSBACKSLASH:
			return 0x61;
		case SDL_SCANCODE_KP_ENTER:
			return 0x5a | EXTENDED_FLAG;
		case SDL_SCANCODE_KP_0:
			return 0x70;
		case SDL_SCANCODE_KP_1:
			return 0x69;
		case SDL_SCANCODE_KP_2:
			return 0x72;
		case SDL_SCANCODE_KP_3:
			return 0x7a;
		case SDL_SCANCODE_KP_4:
			return 0x6b;
		case SDL_SCANCODE_KP_5:
			return 0x73;
		case SDL_SCANCODE_KP_6:
			return 0x74;
		case SDL_SCANCODE_KP_7:
			return 0x6c;
		case SDL_SCANCODE_KP_8:
			return 0x75;
		case SDL_SCANCODE_KP_9:
			return 0x7d;
		case SDL_SCANCODE_KP_PERIOD:
			return 0x71;
		case SDL_SCANCODE_KP_PLUS:
			return 0x79;
		case SDL_SCANCODE_KP_MINUS:
			return 0x7b;
		case SDL_SCANCODE_KP_MULTIPLY:
			return 0x7c;
		case SDL_SCANCODE_KP_DIVIDE:
			return 0x4a | EXTENDED_FLAG;
		default:
			return 0;
	}
}

static uint8_t
get_pixel(uint8_t layer, uint16_t x, uint16_t y)
{
	uint8_t enabled = reg_layer[layer][0] & 1;
	if (!enabled) {
		return 0; // transparent
	}

	uint8_t mode = reg_layer[layer][0] >> 5;
	uint32_t map_base = reg_layer[layer][2] << 2 | reg_layer[layer][3] << 10;
	uint32_t tile_base = reg_layer[layer][4] << 2 | reg_layer[layer][5] << 10;

	bool text_mode = mode == 0 || mode == 1;
	bool tile_mode = mode == 2 || mode == 3 || mode == 4;
	bool bitmap_mode = mode == 5 || mode == 6 || mode == 7;

	uint16_t mapw = 0;
	uint16_t maph = 0;
	uint16_t tilew = 0;
	uint16_t tileh = 0;

	if (tile_mode || text_mode) {
		mapw = 1 << ((reg_layer[layer][1] & 3) + 5);
		maph = 1 << (((reg_layer[layer][1] >> 2) & 3) + 5);
		if (tile_mode) {
			tilew = 1 << (((reg_layer[layer][1] >> 4) & 1) + 3);
			tileh = 1 << (((reg_layer[layer][1] >> 5) & 1) + 3);
		} else {
			tilew = 8;
			tileh = 8;
		}
	} else if (bitmap_mode) {
		// bitmap mode is basically tiled mode with a single huge tile
		tilew = ((reg_layer[layer][1] >> 4) & 1) ? 640 : 320;
		tileh = SCREEN_HEIGHT;
	}

	// Scrolling
	if (!bitmap_mode) {
		uint16_t hscroll = reg_layer[layer][6] | (reg_layer[layer][7] & 0xf) << 8;
		uint16_t vscroll = reg_layer[layer][8] | (reg_layer[layer][9] & 0xf) << 8;

		x = (x + hscroll) % (mapw * tilew);
		y = (y + vscroll) % (maph * tileh);
	}

	int xx = x % tilew;
	int yy = y % tileh;

	uint16_t tile_index = 0;
	uint8_t fg_color = 0;
	uint8_t bg_color = 0;
	uint8_t palette_offset = 0;

	// extract all information from the map
	if (bitmap_mode) {
		tile_index = 0;
		palette_offset = reg_layer[layer][7] & 0xf;
	} else {
		uint32_t map_addr = map_base + (y / tileh * mapw + x / tilew) * 2;
		uint8_t byte0 = video_space_read(map_addr);
		uint8_t byte1 = video_space_read(map_addr + 1);
		if (text_mode) {
			tile_index = byte0;

			if (mode == 0) {
				fg_color = byte1 & 15;
				bg_color = byte1 >> 4;
			} else {
				fg_color = byte1;
				bg_color = 0;
			}
		} else if (tile_mode) {
			tile_index = byte0 | ((byte1 & 3) << 8);

			// Tile Flipping
			bool vflip = (byte1 >> 3) & 1;
			bool hflip = (byte1 >> 2) & 1;
			if (vflip) {
				yy = yy ^ (tileh - 1);
			}
			if (hflip) {
				xx = xx ^ (tilew - 1);
			}

			palette_offset = byte1 >> 4;
		}
	}

	uint8_t bits_per_pixel = 0;
	if (mode == 0 || mode == 1) {
		bits_per_pixel = 1;
	} else if (mode == 2 || mode == 5) {
		bits_per_pixel = 2;
	} else if (mode == 3 || mode == 6) {
		bits_per_pixel = 4;
	} else if (mode == 4 || mode == 7) {
		bits_per_pixel = 8;
	}

	uint32_t tile_size = (tilew * bits_per_pixel * tileh) >> 3;
	// offset within tilemap of the current tile
	uint32_t tile_start = tile_index * tile_size;
	// additional bytes to reach the correct line of the tile
	uint32_t y_add = (yy * tilew * bits_per_pixel) >> 3;
	// additional bytes to reach the correct column of the tile
	uint16_t x_add = (xx * bits_per_pixel) >> 3;
	uint32_t tile_offset = tile_start + y_add + x_add;
	uint8_t s = video_space_read(tile_base + tile_offset);

	// convert tile byte to indexed color
	uint8_t col_index = 0;
	if (bits_per_pixel == 1) {
		bool bit = (s >> (7 - xx)) & 1;
		col_index = bit ? fg_color : bg_color;
	} else if (bits_per_pixel == 2) {
		col_index = (s >> (6 - ((xx & 3) << 1))) & 3;
	} else if (bits_per_pixel == 4) {
		col_index = (s >> (4 - ((xx & 1) << 2))) & 0xf;
	} else if (bits_per_pixel == 8) {
		col_index = s;
	}

	// Apply Palette Offset
	if (palette_offset && col_index > 0 && col_index < 16) {
		col_index += palette_offset << 4;
	}

	return col_index;
}

uint8_t
get_sprite(uint16_t x, uint16_t y)
{
	if (!(reg_sprites[0] & 1)) {
		// sprites disabled
		return 0;
	}
	for (int i = 0; i < NUM_SPRITES; i++) {
		int8_t sprite_zdepth = (sprite_data[i][6] >> 2) & 3;
		if (sprite_zdepth == 0) {
			continue;
		}
		int16_t sprite_x = sprite_data[i][2] | (sprite_data[i][3] & 3) << 8;
		int16_t sprite_y = sprite_data[i][4] | (sprite_data[i][5] & 3) << 8;
		uint8_t sprite_width = 1 << (((sprite_data[i][7] >> 4) & 3) + 3);
		uint8_t sprite_height = 1 << ((sprite_data[i][7] >> 6) + 3);

		// fix up negative coordinates
		if (sprite_x >= 0x400 - sprite_width) {
			sprite_x |= 0xff00 - 0x200;
		}
		if (sprite_y >= 0x200 - sprite_height) {
			sprite_y |= 0xff00 - 0x100;
		}

		// check whether this pixel falls within the sprite
		if (x < sprite_x || x >= sprite_x + sprite_width) {
			continue;
		}
		if (y < sprite_y || y >= sprite_y + sprite_height) {
			continue;
		}

		// relative position within the sprite
		uint16_t sx = x - sprite_x;
		uint16_t sy = y - sprite_y;

		// flip
		if (sprite_data[i][6] & 1) {
			sx = sprite_width - sx;
		}
		if ((sprite_data[i][6] >> 1) & 1) {
			sy = sprite_height - sy;
		}

		bool mode = (sprite_data[i][1] >> 7) & 1;
		uint32_t sprite_address = sprite_data[i][0] << 5 | (sprite_data[i][1] & 0xf) << 13;

		uint8_t col_index = 0;
		if (!mode) {
			// 4 bpp
			uint8_t byte = video_ram[sprite_address +  (sy * sprite_width>>1) + (sx>>1)];
			if (sx & 1) {
				col_index = byte & 0xf;
			} else {
				col_index = byte >> 4;
			}
		} else {
			// 8 bpp
			col_index = video_ram[sprite_address + sy * sprite_width + sx];
		}
		// palette offset
		if (col_index > 0) {
			col_index += (sprite_data[i][7] & 0x0f) << 4;
			return col_index;
		}
	}
	return 0;
}

float start_scan_pixel_pos, end_scan_pixel_pos;

static void
video_flush_internal(int start, int end)
{
	uint8_t out_mode = reg_composer[0] & 3;
	bool chroma_disable = (reg_composer[0] >> 2) & 1;

	float hscale = 128.0 / reg_composer[1];
	float vscale = 128.0 / reg_composer[2];

	uint8_t border_color = reg_composer[3];
	uint16_t hstart = reg_composer[4] | (reg_composer[8] & 3) << 8;
	uint16_t hstop = reg_composer[5] | ((reg_composer[8] >> 2) & 3) << 8;
	uint16_t vstart = reg_composer[6] | ((reg_composer[8] >> 4) & 1) << 8;
	uint16_t vstop = reg_composer[7] | ((reg_composer[8] >> 5) & 1) << 8;

	for (int pp = start; pp < end; pp++) {
		int x;
		int y;
		if (out_mode == 0 || out_mode == 1) {
			// VGA
			x = pp % SCAN_WIDTH;
			y = pp / SCAN_WIDTH;
			x -= VGA_FRONT_PORCH_X;
			y -= VGA_FRONT_PORCH_Y;
		} else {
			// NTSC
			int pp2 = pp;
			int field = pp2 > SCAN_WIDTH * SCAN_HEIGHT / 2;
			if (field == 1) {
				pp2 -= SCAN_WIDTH * SCAN_HEIGHT / 2;
			}
			x = pp2 % SCAN_WIDTH;
			y = pp2 / SCAN_WIDTH * 2 + (1 - field);
			x -= NTSC_FRONT_PORCH_X;
			y -= NTSC_FRONT_PORCH_Y;
		}

		if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
			continue;
		}

		int eff_x = 1.0 / hscale * (x - hstart);
		int eff_y = 1.0 / vscale * (y - vstart);

		uint8_t r;
		uint8_t g;
		uint8_t b;

		if (out_mode == 0) {
			// video generation off
			// -> show blue screen
			r = 0;
			g = 0;
			b = 255;
		} else {
			uint8_t col_index;
			if (x < hstart || x > hstop || y < vstart || y > vstop) {
				col_index = border_color;
			} else {
				col_index = get_pixel(1, eff_x, eff_y);
				if (col_index == 0) { // Layer 2 is transparent
					col_index = get_pixel(0, eff_x, eff_y);
				}
				uint8_t spr_col_index = get_sprite(eff_x, eff_y);
				if (spr_col_index) {
					col_index = spr_col_index;
				}
			}

			uint16_t entry = palette[col_index * 2] | palette[col_index * 2 + 1] << 8;
			r = ((entry >> 8) & 0xf) << 4;
			g = ((entry >> 4) & 0xf) << 4;
			b = (entry & 0xf) << 4;
			if (chroma_disable) {
				r = g = b = (r + b + g) / 3;
			}

			// NTSC overscan
			if (out_mode == 2) {
				if (x < SCREEN_WIDTH * TITLE_SAFE_X ||
				    x > SCREEN_WIDTH * (1 - TITLE_SAFE_X) ||
				    y < SCREEN_HEIGHT * TITLE_SAFE_Y ||
				    y > SCREEN_HEIGHT * (1 - TITLE_SAFE_Y)) {
#if 1
					r /= 3;
					g /= 3;
					b /= 3;
#else
					r ^= 128;
					g ^= 128;
					b ^= 128;
#endif

				}
			}
		}
		int fbi = (y * SCREEN_WIDTH + x) * 4;
		framebuffer[fbi + 0] = b;
		framebuffer[fbi + 1] = g;
		framebuffer[fbi + 2] = r;
	}
}

bool
video_step(float mhz)
{
	uint8_t out_mode = reg_composer[0] & 3;

	bool new_frame = false;
	if (out_mode == 0 || out_mode == 1) {
		end_scan_pixel_pos += VGA_PIXEL_FREQ / mhz;
	} else {
		end_scan_pixel_pos += NTSC_PIXEL_FREQ / mhz;
	}
	if (end_scan_pixel_pos >= SCAN_WIDTH * SCAN_HEIGHT) {
		new_frame = true;
		int start = (int)floor(start_scan_pixel_pos);
		int end = SCAN_WIDTH * SCAN_HEIGHT;
		video_flush_internal(start, end);
		start_scan_pixel_pos = 0;
		end_scan_pixel_pos = 0;
		if (ien & 1) { // VSYNC
			isr |= 1;
		}
	}

	return new_frame;
}

bool
video_get_irq_out()
{
	return isr > 0;
}

static void
video_flush()
{
	int start = (int)floor(start_scan_pixel_pos);
	int end = (int)floor(end_scan_pixel_pos);
	video_flush_internal(start, end);
	start_scan_pixel_pos = end_scan_pixel_pos;
}

bool
video_update()
{
	SDL_UpdateTexture(sdlTexture, NULL, framebuffer, SCREEN_WIDTH * 4);

	if (record_gif) {
		record_gif = GifWriteFrame(&gif_writer, framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, 2, 8, false);
		if(!record_gif) {
			GifEnd(&gif_writer);
		}
	}

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);

	if (debuger_enabled && showDebugOnRender != 0) {
		DEBUGRenderDisplay(SCREEN_WIDTH,SCREEN_HEIGHT,renderer);
		SDL_RenderPresent(renderer);
		return true;
	}

	SDL_RenderPresent(renderer);

	static bool cmd_down = false;
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			return false;
		}
		if (event.type == SDL_KEYDOWN) {
			bool consumed = false;
			if (cmd_down) {
				if (event.key.keysym.sym == SDLK_s) {
					memory_save();
					consumed = true;
				} else if (event.key.keysym.sym == SDLK_r) {
					machine_reset();
					consumed = true;
				} else if (event.key.keysym.sym == SDLK_v) {
					machine_paste(SDL_GetClipboardText());
					consumed = true;
				} else if (event.key.keysym.sym == SDLK_f ||  event.key.keysym.sym == SDLK_RETURN) {
					is_fullscreen = !is_fullscreen;
					SDL_SetWindowFullscreen(window, is_fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
					consumed = true;
				}
			}
			if (!consumed) {
				if (log_keyboard) {
					printf("DOWN 0x%02x\n", event.key.keysym.scancode);
				}
				if (event.key.keysym.scancode == LSHORTCUT_KEY || event.key.keysym.scancode == RSHORTCUT_KEY) {
					cmd_down = true;
				}

				int scancode = ps2_scancode_from_SDLKey(event.key.keysym.scancode);
				if (scancode == 0xff) {
					// "Pause/Break" sequence
					kbd_buffer_add(0xe1);
					kbd_buffer_add(0x14);
					kbd_buffer_add(0x77);
					kbd_buffer_add(0xe1);
					kbd_buffer_add(0xf0);
					kbd_buffer_add(0x14);
					kbd_buffer_add(0xf0);
					kbd_buffer_add(0x77);
				} else {
					if (scancode & EXTENDED_FLAG) {
						kbd_buffer_add(0xe0);
					}
					kbd_buffer_add(scancode & 0xff);
				}
			}
			return true;
		}
		if (event.type == SDL_KEYUP) {
			if (log_keyboard) {
				printf("UP   0x%02x\n", event.key.keysym.scancode);
			}
			if (event.key.keysym.scancode == LSHORTCUT_KEY || event.key.keysym.scancode == RSHORTCUT_KEY) {
				cmd_down = false;
			}

			int scancode = ps2_scancode_from_SDLKey(event.key.keysym.scancode);
			if (scancode & EXTENDED_FLAG) {
				kbd_buffer_add(0xe0);
			}
			kbd_buffer_add(0xf0); // BREAK
			kbd_buffer_add(scancode & 0xff);
			return true;
		}
		if (event.type == SDL_MOUSEBUTTONDOWN) {
			switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					mouse_button_down(0);
					break;
				case SDL_BUTTON_RIGHT:
					mouse_button_down(1);
					break;
			}
		}
		if (event.type == SDL_MOUSEBUTTONUP) {
			switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					mouse_button_up(0);
					break;
				case SDL_BUTTON_RIGHT:
					mouse_button_up(1);
					break;
			}
		}
		if (event.type == SDL_MOUSEMOTION) {
			mouse_move(event.motion.x, event.motion.y);
		}
	}
	return true;
}

void
video_end()
{
	if (record_gif) {
		GifEnd(&gif_writer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

uint32_t
get_and_inc_address(uint8_t sel)
{
	uint32_t address = io_addr[sel];
#ifdef VERA_V0_8
	if (io_inc[sel]) {
		io_addr[sel] += 1 << (io_inc[sel] - 1);
	}
#else
	io_addr[sel] += io_inc[sel];
#endif
//	printf("address = %06x, new = %06x\n", address, io_addr[sel]);
	return address;
}

//
// Vera: Internal Video Address Space
//

static uint8_t
video_space_read(uint32_t address)
{
	if (address >= ADDR_VRAM_START && address < ADDR_VRAM_END) {
		return video_ram[address];
#ifndef VERA_V0_8
	} else if (address >= ADDR_CHARSET_START && address < ADDR_CHARSET_END) {
		return chargen_rom[address & 0xfff];
#endif
	} else if (address >= ADDR_LAYER1_START && address < ADDR_LAYER1_END) {
		return reg_layer[0][address & 0xf];
	} else if (address >= ADDR_LAYER2_START && address < ADDR_LAYER2_END) {
		return reg_layer[1][address & 0xf];
	} else if (address >= ADDR_SPRITES_START && address < ADDR_SPRITES_END) {
		return reg_sprites[address & 0xf];
	} else if (address >= ADDR_COMPOSER_START && address < ADDR_COMPOSER_END) {
		return reg_composer[address & 0x1f];
	} else if (address >= ADDR_PALETTE_START && address < ADDR_PALETTE_END) {
		return palette[address & 0x1ff];
	} else if (address >= ADDR_SPRDATA_START && address < ADDR_SPRDATA_END) {
		return sprite_data[(address >> 3) & 0xff][address & 0x7];
	} else if (address >= ADDR_SPI_START && address < ADDR_SPI_END) {
		return vera_spi_read(address & 1);
	} else {
		return 0xFF; // unassigned
	}
}

void
video_space_write(uint32_t address, uint8_t value)
{
	video_flush();
	if (address >= ADDR_VRAM_START && address < ADDR_VRAM_END) {
		video_ram[address] = value;
#ifndef VERA_V0_8
	} else if (address >= ADDR_CHARSET_START && address < ADDR_CHARSET_END) {
		// ROM, do nothing
#endif
	} else if (address >= ADDR_LAYER1_START && address < ADDR_LAYER1_END) {
		reg_layer[0][address & 0xf] = value;
	} else if (address >= ADDR_LAYER2_START && address < ADDR_LAYER2_END) {
		reg_layer[1][address & 0xf] = value;
	} else if (address >= ADDR_SPRITES_START && address < ADDR_SPRITES_END) {
		reg_sprites[address & 0xf] = value;
	} else if (address >= ADDR_COMPOSER_START && address < ADDR_COMPOSER_END) {
		reg_composer[address & 0x1f] = value;
	} else if (address >= ADDR_PALETTE_START && address < ADDR_PALETTE_END) {
		palette[address & 0x1ff] = value;
	} else if (address >= ADDR_SPRDATA_START && address < ADDR_SPRDATA_END) {
		sprite_data[(address >> 3) & 0xff][address & 0x7] = value;
	} else if (address >= ADDR_SPI_START && address < ADDR_SPI_END) {
		vera_spi_write(address & 1, value);
	} else {
		// unassigned, do nothing
	}
}

//
// Vera: 6502 I/O Interface
//

uint8_t
video_read(uint8_t reg)
{
	switch (reg) {
#ifdef VERA_V0_8
		case 2:
#else
		case 0:
#endif
			return (io_addr[io_addrsel] >> 16) | (io_inc[io_addrsel] << 4);
		case 1:
			return (io_addr[io_addrsel] >> 8) & 0xff;
#ifdef VERA_V0_8
		case 0:
#else
		case 2:
#endif
			return io_addr[io_addrsel] & 0xff;
		case 3:
		case 4: {
			uint32_t address = get_and_inc_address(reg - 3);
			uint8_t value = video_space_read(address);
			if (log_video) {
				printf("READ  video_space[$%x] = $%02x\n", address, value);
			}
			return value;
		case 5:
			return io_addrsel;
		case 6:
			return ien;
		case 7:
			return isr;
		default:
			return 0;
		}
	}
}

void
video_write(uint8_t reg, uint8_t value)
{
//	printf("ioregisters[%d] = $%02x\n", reg, value);
	switch (reg) {
#ifdef VERA_V0_8
		case 2:
#else
		case 0:
#endif
			io_addr[io_addrsel] = (io_addr[io_addrsel] & 0x0ffff) | ((value & 0xf) << 16);
			io_inc[io_addrsel] = value >> 4;
			break;
		case 1:
			io_addr[io_addrsel] = (io_addr[io_addrsel] & 0xf00ff) | (value << 8);
			break;
#ifdef VERA_V0_8
		case 0:
#else
		case 2:
#endif
			io_addr[io_addrsel] = (io_addr[io_addrsel] & 0xfff00) | value;
			break;
		case 3:
		case 4: {
			uint32_t address = get_and_inc_address(reg - 3);
			if (log_video) {
				printf("WRITE video_space[$%x] = $%02x\n", address, value);
			}
			video_space_write(address, value);
			break;
		case 5:
			if (value & 0x80) {
				video_reset();
			}
			io_addrsel = value  & 1;
			break;
		case 6:
			ien = value;
			break;
		case 7:
			isr &= value ^ 0xff;
			break;
		}
	}
}
