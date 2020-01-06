// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include "video.h"
#include "memory.h"
#include "ps2.h"
#include "glue.h"
#include "debugger.h"
#include "keyboard.h"
#include "gif.h"
#include "vera_spi.h"
#include "vera_uart.h"

#include <limits.h>

#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#endif

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
#define ADDR_SPI_START      0xF7000
#define ADDR_SPI_END        0xF8000
#define ADDR_UART_START     0xF8000
#define ADDR_UART_END       0xF9000

#define NUM_SPRITES 128

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

// When rendering a layer line, we can amortize some of the cost by calculating multiple pixels at a time.
#define LAYER_PIXELS_PER_ITERATION 8


static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *sdlTexture;
static bool is_fullscreen = false;

static uint8_t video_ram[0x20000];
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

static uint8_t layer_line[2][SCREEN_WIDTH];
static uint8_t sprite_line_col[SCREEN_WIDTH];
static uint8_t sprite_line_z[SCREEN_WIDTH];
static bool layer_line_empty[2];
static bool sprite_line_empty;

float scan_pos_x;
uint16_t scan_pos_y;

static uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT * 4];

static GifWriter gif_writer;

static const uint16_t default_palette[] = {
0x000,0xfff,0x800,0xafe,0xc4c,0x0c5,0x00a,0xee7,0xd85,0x640,0xf77,0x333,0x777,0xaf6,0x08f,0xbbb,0x000,0x111,0x222,0x333,0x444,0x555,0x666,0x777,0x888,0x999,0xaaa,0xbbb,0xccc,0xddd,0xeee,0xfff,0x211,0x433,0x644,0x866,0xa88,0xc99,0xfbb,0x211,0x422,0x633,0x844,0xa55,0xc66,0xf77,0x200,0x411,0x611,0x822,0xa22,0xc33,0xf33,0x200,0x400,0x600,0x800,0xa00,0xc00,0xf00,0x221,0x443,0x664,0x886,0xaa8,0xcc9,0xfeb,0x211,0x432,0x653,0x874,0xa95,0xcb6,0xfd7,0x210,0x431,0x651,0x862,0xa82,0xca3,0xfc3,0x210,0x430,0x640,0x860,0xa80,0xc90,0xfb0,0x121,0x343,0x564,0x786,0x9a8,0xbc9,0xdfb,0x121,0x342,0x463,0x684,0x8a5,0x9c6,0xbf7,0x120,0x241,0x461,0x582,0x6a2,0x8c3,0x9f3,0x120,0x240,0x360,0x480,0x5a0,0x6c0,0x7f0,0x121,0x343,0x465,0x686,0x8a8,0x9ca,0xbfc,0x121,0x242,0x364,0x485,0x5a6,0x6c8,0x7f9,0x020,0x141,0x162,0x283,0x2a4,0x3c5,0x3f6,0x020,0x041,0x061,0x082,0x0a2,0x0c3,0x0f3,0x122,0x344,0x466,0x688,0x8aa,0x9cc,0xbff,0x122,0x244,0x366,0x488,0x5aa,0x6cc,0x7ff,0x022,0x144,0x166,0x288,0x2aa,0x3cc,0x3ff,0x022,0x044,0x066,0x088,0x0aa,0x0cc,0x0ff,0x112,0x334,0x456,0x668,0x88a,0x9ac,0xbcf,0x112,0x224,0x346,0x458,0x56a,0x68c,0x79f,0x002,0x114,0x126,0x238,0x24a,0x35c,0x36f,0x002,0x014,0x016,0x028,0x02a,0x03c,0x03f,0x112,0x334,0x546,0x768,0x98a,0xb9c,0xdbf,0x112,0x324,0x436,0x648,0x85a,0x96c,0xb7f,0x102,0x214,0x416,0x528,0x62a,0x83c,0x93f,0x102,0x204,0x306,0x408,0x50a,0x60c,0x70f,0x212,0x434,0x646,0x868,0xa8a,0xc9c,0xfbe,0x211,0x423,0x635,0x847,0xa59,0xc6b,0xf7d,0x201,0x413,0x615,0x826,0xa28,0xc3a,0xf3c,0x201,0x403,0x604,0x806,0xa08,0xc09,0xf0b
};

static uint8_t video_space_read(uint32_t address);
static void video_space_read_range(uint8_t* dest, uint32_t address, uint32_t size);

static void refresh_palette();

void
video_reset()
{
	// init I/O registers
	memset(io_addr, 0, sizeof(io_addr));
	memset(io_inc, 0, sizeof(io_inc));
	io_addrsel = 0;

	// init Layer registers
	memset(reg_layer, 0, sizeof(reg_layer));

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

	refresh_palette();

	scan_pos_x = 0;
	scan_pos_y = 0;
}

bool
video_init(int window_scale, char *quality)
{
	video_reset();

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, quality);
	SDL_CreateWindowAndRenderer(SCREEN_WIDTH * window_scale, SCREEN_HEIGHT * window_scale, 0, &window, &renderer);
#ifndef __MORPHOS__
	SDL_SetWindowResizable(window, true);
#endif
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

	sdlTexture = SDL_CreateTexture(renderer,
									SDL_PIXELFORMAT_RGB888,
									SDL_TEXTUREACCESS_STREAMING,
									SCREEN_WIDTH, SCREEN_HEIGHT);

	SDL_SetWindowTitle(window, "Commander X16");

	SDL_ShowCursor(SDL_DISABLE);

	if (record_gif != RECORD_GIF_DISABLED) {
		if (!strcmp(gif_path+strlen(gif_path)-5, ",wait")) {
			// wait for POKE
			record_gif = RECORD_GIF_PAUSED;
			// move the string terminator to remove the ",wait"
			gif_path[strlen(gif_path)-5] = 0;
		} else {
			// start now
			record_gif = RECORD_GIF_ACTIVE;
		}
		if (!GifBegin(&gif_writer, gif_path, SCREEN_WIDTH, SCREEN_HEIGHT, 1, 8, false)) {
			record_gif = RECORD_GIF_DISABLED;
		}
	}

	if (debugger_enabled) {
		DEBUGInitUI(renderer);
	}

	return true;
}

struct video_layer_properties
{
	bool enabled;

	uint8_t mode;
	uint32_t map_base;
	uint32_t tile_base;

	bool text_mode;
	bool tile_mode;
	bool bitmap_mode;

	uint16_t hscroll;
	uint16_t vscroll;

	uint16_t mapw;
	uint16_t maph;
	uint16_t tilew;
	uint16_t tileh;

	uint16_t mapw_max;
	uint16_t maph_max;
	uint16_t tilew_max;
	uint16_t tileh_max;

	uint16_t layerw;
	uint16_t layerh;

	uint16_t layerw_max;
	uint16_t layerh_max;

	uint8_t bits_per_pixel;

	uint32_t tile_size;

	int min_eff_x;
	int max_eff_x;
};

struct video_layer_properties layer_properties[2];

static int
calc_layer_eff_x(struct video_layer_properties *props, int x)
{
	return (x + props->hscroll) & (props->layerw_max);
}

static int
calc_layer_eff_y(struct video_layer_properties *props, int y)
{
	return (y + props->vscroll) & (props->layerh_max);
}

static uint32_t
calc_layer_map_addr(struct video_layer_properties *props, int eff_x, int eff_y)
{
	return props->map_base + (eff_y / props->tileh * props->mapw + eff_x / props->tilew) * 2;
}

void refresh_layer_properties(uint8_t layer)
{
	struct video_layer_properties* props = &layer_properties[layer];

	uint16_t prev_layerw_max = props->layerw_max;
	uint16_t prev_hscroll = props->hscroll;

	props->enabled = reg_layer[layer][0] & 1;

	props->mode = reg_layer[layer][0] >> 5;
	props->map_base = reg_layer[layer][2] << 2 | reg_layer[layer][3] << 10;
	props->tile_base = reg_layer[layer][4] << 2 | reg_layer[layer][5] << 10;

	props->text_mode = (props->mode == 0) || (props->mode == 1);
	props->tile_mode = (props->mode == 2) || (props->mode == 3) || (props->mode == 4);
	props->bitmap_mode = (props->mode == 5) || (props->mode == 6) || (props->mode == 7);

	if (!props->bitmap_mode) {
		props->hscroll = reg_layer[layer][6] | (reg_layer[layer][7] & 0xf) << 8;
		props->vscroll = reg_layer[layer][8] | (reg_layer[layer][9] & 0xf) << 8;
	}

	props->mapw = 0;
	props->maph = 0;
	props->tilew = 0;
	props->tileh = 0;

	if (props->tile_mode || props->text_mode) {
		props->mapw = 1 << ((reg_layer[layer][1] & 3) + 5);
		props->maph = 1 << (((reg_layer[layer][1] >> 2) & 3) + 5);
		// Scale the tiles or text characters arccording to TILEW and TILEH.
		props->tilew = 1 << (((reg_layer[layer][1] >> 4) & 1) + 3);
		props->tileh = 1 << (((reg_layer[layer][1] >> 5) & 1) + 3);
	} else if (props->bitmap_mode) {
		// bitmap mode is basically tiled mode with a single huge tile
		props->tilew = ((reg_layer[layer][1] >> 4) & 1) ? 640 : 320;
		props->tileh = SCREEN_HEIGHT;
	}

	// We know mapw, maph, tilew, and tileh are powers of two, and any products of that set will be powers of two,
	// so there's no need to modulo against them if we have bitmasks we can bitwise-and against.

	props->mapw_max = props->mapw - 1;
	props->maph_max = props->maph - 1;
	props->tilew_max = props->tilew - 1;
	props->tileh_max = props->tileh - 1;

	props->layerw = props->mapw * props->tilew;
	props->layerh = props->maph * props->tileh;

	props->layerw_max = props->layerw - 1;
	props->layerh_max = props->layerh - 1;

	// Find min/max eff_x for bulk reading in tile data during draw.
	if (prev_layerw_max != props->layerw_max || prev_hscroll != props->hscroll) {
		int min_eff_x = INT_MAX;
		int max_eff_x = INT_MIN;
		for (int x = 0; x < SCREEN_WIDTH; ++x) {
			int eff_x = calc_layer_eff_x(props, x);
			if (eff_x < min_eff_x) {
				min_eff_x = eff_x;
			}
			if (eff_x > max_eff_x) {
				max_eff_x = eff_x;
			}
		}
		props->min_eff_x = min_eff_x;
		props->max_eff_x = max_eff_x;
	}

	props->bits_per_pixel = 0;
	if ((props->mode == 0) || (props->mode == 1)) {
		props->bits_per_pixel = 1;
	} else if ((props->mode == 2) || (props->mode == 5)) {
		props->bits_per_pixel = 2;
	} else if ((props->mode == 3) || (props->mode == 6)) {
		props->bits_per_pixel = 4;
	} else if ((props->mode == 4) || (props->mode == 7)) {
		props->bits_per_pixel = 8;
	}

	props->tile_size = (props->tilew * props->bits_per_pixel * props->tileh) >> 3;
}

struct video_sprite_properties
{
	int8_t sprite_zdepth;

	int16_t sprite_x;
	int16_t sprite_y;
	uint8_t sprite_width;
	uint8_t sprite_height;

	bool hflip;
	bool vflip;

	bool mode;
	uint32_t sprite_address;

	uint16_t palette_offset;
};

struct video_sprite_properties sprite_properties[256];

void refresh_sprite_properties(uint16_t sprite)
{
	struct video_sprite_properties* props = &sprite_properties[sprite];

	props->sprite_zdepth = (sprite_data[sprite][6] >> 2) & 3;

	props->sprite_x = sprite_data[sprite][2] | (sprite_data[sprite][3] & 3) << 8;
	props->sprite_y = sprite_data[sprite][4] | (sprite_data[sprite][5] & 3) << 8;
	props->sprite_width = 1 << (((sprite_data[sprite][7] >> 4) & 3) + 3);
	props->sprite_height = 1 << ((sprite_data[sprite][7] >> 6) + 3);

	// fix up negative coordinates
	if (props->sprite_x >= 0x400 - props->sprite_width) {
		props->sprite_x |= 0xff00 - 0x200;
	}
	if (props->sprite_y >= 0x200 - props->sprite_height) {
		props->sprite_y |= 0xff00 - 0x100;
	}

	props->hflip = sprite_data[sprite][6] & 1;
	props->vflip = (sprite_data[sprite][6] >> 1) & 1;

	props->mode = (sprite_data[sprite][1] >> 7) & 1;
	props->sprite_address = sprite_data[sprite][0] << 5 | (sprite_data[sprite][1] & 0xf) << 13;

	props->palette_offset = (sprite_data[sprite][7] & 0x0f) << 4;
}

struct video_palette
{
	uint32_t entries[256];
	bool dirty;
};

struct video_palette video_palette;

static void refresh_palette() {
	uint8_t out_mode = reg_composer[0] & 3;
	bool chroma_disable = (reg_composer[0] >> 2) & 1;
	for (int i = 0; i < 256; ++i) {
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
			uint16_t entry = palette[i * 2] | palette[i * 2 + 1] << 8;
			r = ((entry >> 8) & 0xf) << 4 | ((entry >> 8) & 0xf);
			g = ((entry >> 4) & 0xf) << 4 | ((entry >> 4) & 0xf);
			b = (entry & 0xf) << 4 | (entry & 0xf);
			if (chroma_disable) {
				r = g = b = (r + b + g) / 3;
			}
		}

		video_palette.entries[i] = (uint32_t)(r << 16) | ((uint32_t)g << 8) | ((uint32_t)b);
	}
	video_palette.dirty = false;
}

static void
render_sprite_line(uint16_t y)
{
	if (!(reg_sprites[0] & 1)) {
		// sprites disabled
		sprite_line_empty = true;
		return;
	}
	sprite_line_empty = false;
	for (int i = 0; i < SCREEN_WIDTH; i++) {
		sprite_line_col[i] = 0;
		sprite_line_z[i] = 0;
	}
	uint16_t sprite_budget = 800 + 1;
	for (int i = 0; i < NUM_SPRITES; i++) {
		// one clock per lookup
		sprite_budget--; if (sprite_budget == 0) break;
		struct video_sprite_properties *props = &sprite_properties[i];

		if (props->sprite_zdepth == 0) {
			continue;
		}

		// check whether this line falls within the sprite
		if (y < props->sprite_y || y >= props->sprite_y + props->sprite_height) {
			continue;
		}

		for (uint16_t sx = 0; sx < props->sprite_width; sx++) {
			uint16_t line_x = props->sprite_x + sx;
			if (line_x >= SCREEN_WIDTH) {
				continue;
			}
			uint16_t eff_sx = sx;
			uint16_t eff_sy = y - props->sprite_y;

			// flip
			if (props->hflip) {
				eff_sx = (props->sprite_width - 1) - eff_sx;
			}
			if (props->vflip) {
				eff_sy = (props->sprite_height - 1) - eff_sy;
			}

			uint8_t col_index = 0;
			uint32_t vaddr;
			if (props->mode == 0) {
				// 4 bpp
				vaddr = props->sprite_address + (eff_sy * props->sprite_width >> 1) + (eff_sx >> 1);
				uint8_t byte = video_ram[vaddr];
				if (eff_sx & 1) {
					col_index = byte & 0xf;
				} else {
					col_index = byte >> 4;
				}
			} else {
				// 8 bpp
				vaddr = props->sprite_address + eff_sy * props->sprite_width + eff_sx;
				col_index = video_ram[vaddr];
			}

			// one clock per fetched 32 bits
			if (!(vaddr & 3)) {
				sprite_budget--; if (sprite_budget == 0) break;
			}
			// one clock per rendered pixel
			sprite_budget--; if (sprite_budget == 0) break;
			// palette offset
			if (col_index > 0) {
				col_index += props->palette_offset;
				if (props->sprite_zdepth > sprite_line_z[line_x]) {
					sprite_line_col[line_x] = col_index;
					sprite_line_z[line_x] = props->sprite_zdepth;
				}
			}
		}
	}
}

static void
render_layer_line(uint8_t layer, uint16_t y)
{
	struct video_layer_properties *props = &layer_properties[layer];

	if (!props->enabled) {
		layer_line_empty[layer] = true;
	} else {
		layer_line_empty[layer] = false;

		// Load in tile bytes if not in bitmap mode.
		uint8_t tile_bytes[512]; // max 256 tiles, 2 bytes each.
		uint32_t map_addr_begin = 0;
		uint32_t map_addr_end = 0;

		if (!props->bitmap_mode) {
			int size;
			int eff_y = calc_layer_eff_y(props, y);
			map_addr_begin = calc_layer_map_addr(props, props->min_eff_x, eff_y);
			map_addr_end = calc_layer_map_addr(props, props->max_eff_x, eff_y);
			size = (map_addr_end - map_addr_begin) + 2;

			video_space_read_range(tile_bytes, map_addr_begin, size);
		}

		// Render tile line.
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			uint8_t col_index = 0;
			int xx, yy;

			int eff_x = x;
			int eff_y = y;

			// Scrolling
			if (!props->bitmap_mode) {
				eff_x = calc_layer_eff_x(props, x);
				eff_y = calc_layer_eff_y(props, y);
			}

			if (props->bitmap_mode) {
				xx = eff_x % props->tilew;
				yy = eff_y % props->tileh;
			} else {
				xx = eff_x & props->tilew_max;
				yy = eff_y & props->tileh_max;
			}

			uint16_t tile_index = 0;
			uint8_t fg_color = 0;
			uint8_t bg_color = 0;
			uint8_t palette_offset = 0;

			// extract all information from the map
			if (props->bitmap_mode) {
				tile_index = 0;
				palette_offset = reg_layer[layer][7] & 0xf;
			} else {
				uint32_t map_addr = calc_layer_map_addr(props, eff_x, eff_y) - map_addr_begin;

				uint8_t byte0 = tile_bytes[map_addr];
				uint8_t byte1 = tile_bytes[map_addr + 1];

				if (props->text_mode) {
					tile_index = byte0;

					if (props->mode == 0) {
						fg_color = byte1 & 15;
						bg_color = byte1 >> 4;
					} else {
						fg_color = byte1;
						bg_color = 0;
					}
				} else if (props->tile_mode) {
					tile_index = byte0 | ((byte1 & 3) << 8);

					// Tile Flipping
					bool vflip = (byte1 >> 3) & 1;
					bool hflip = (byte1 >> 2) & 1;
					if (vflip) {
						yy = yy ^ (props->tileh - 1);
					}
					if (hflip) {
						xx = xx ^ (props->tilew - 1);
					}

					palette_offset = byte1 >> 4;
				}
			}

			// offset within tilemap of the current tile
			uint32_t tile_start = tile_index * props->tile_size;
			// additional bytes to reach the correct line of the tile
			uint32_t y_add = (yy * props->tilew * props->bits_per_pixel) >> 3;
			// additional bytes to reach the correct column of the tile
			uint16_t x_add = (xx * props->bits_per_pixel) >> 3;
			uint32_t tile_offset = tile_start + y_add + x_add;
			uint8_t s = video_space_read(props->tile_base + tile_offset);

			// convert tile byte to indexed color
			if (props->bits_per_pixel == 1) {
				bool bit = (s >> (7 - xx)) & 1;
				col_index = bit ? fg_color : bg_color;
			} else if (props->bits_per_pixel == 2) {
				col_index = (s >> (6 - ((xx & 3) << 1))) & 3;
			} else if (props->bits_per_pixel == 4) {
				col_index = (s >> (4 - ((xx & 1) << 2))) & 0xf;
			} else if (props->bits_per_pixel == 8) {
				col_index = s;
			}

			// Apply Palette Offset
			if (palette_offset && col_index > 0 && col_index < 16) {
				col_index += palette_offset << 4;
			}
			layer_line[layer][x] = col_index;
		}
	}
}

static uint8_t calculate_line_col_index(uint8_t spr_zindex, uint8_t spr_col_index, uint8_t l1_col_index, uint8_t l2_col_index)
{
	uint8_t col_index = 0;
	switch (spr_zindex) {
		case 3:
			col_index = spr_col_index ?: l2_col_index ?: l1_col_index;
			break;
		case 2:
			col_index = l2_col_index ?: spr_col_index ?: l1_col_index;
			break;
		case 1:
			col_index = l2_col_index ?: l1_col_index ?: spr_col_index;
			break;
		case 0:
			col_index = l2_col_index ?: l1_col_index;
			break;
	}
	return col_index;
}

static void
render_line(uint16_t y)
{
	uint8_t out_mode = reg_composer[0] & 3;

	uint8_t border_color = reg_composer[3];
	uint16_t hstart = reg_composer[4] | (reg_composer[8] & 3) << 8;
	uint16_t hstop = reg_composer[5] | ((reg_composer[8] >> 2) & 3) << 8;
	uint16_t vstart = reg_composer[6] | ((reg_composer[8] >> 4) & 1) << 8;
	uint16_t vstop = reg_composer[7] | ((reg_composer[8] >> 5) & 1) << 8;

	int eff_y = (reg_composer[2] * (y - vstart)) / 128;
	render_sprite_line(eff_y);
	render_layer_line(0, eff_y);
	render_layer_line(1, eff_y);

	uint8_t col_line[SCREEN_WIDTH];

	if (video_palette.dirty) {
		refresh_palette();
	}

	// If video output is enabled, calculate color indices for line.
	if (out_mode != 0) {
		uint8_t spr_col_index[LAYER_PIXELS_PER_ITERATION];
		uint8_t l1_col_index[LAYER_PIXELS_PER_ITERATION];
		uint8_t l2_col_index[LAYER_PIXELS_PER_ITERATION];
		uint8_t spr_zindex[LAYER_PIXELS_PER_ITERATION];

		memset(spr_col_index, 0, sizeof(spr_col_index));
		memset(l1_col_index, 0, sizeof(l1_col_index));
		memset(l2_col_index, 0, sizeof(l2_col_index));
		memset(spr_zindex, 0, sizeof(spr_zindex));

		// Calculate color without border.
		for (uint16_t x = 0; x < SCREEN_WIDTH; x+=LAYER_PIXELS_PER_ITERATION) {
			uint8_t col_index[LAYER_PIXELS_PER_ITERATION];
			memset(col_index, 0, sizeof(col_index));

			int eff_x[LAYER_PIXELS_PER_ITERATION];
			for (int i = 0; i < LAYER_PIXELS_PER_ITERATION; ++i) {
				eff_x[i] = (reg_composer[1] * (x + i - hstart)) / 128;
			}

			if (!sprite_line_empty) {
				for (int i = 0; i < LAYER_PIXELS_PER_ITERATION; ++i) {
					spr_col_index[i] = sprite_line_col[eff_x[i]];
				}
			}

			if (!layer_line_empty[0]) {
				for (int i = 0; i < LAYER_PIXELS_PER_ITERATION; ++i) {
					l1_col_index[i] = layer_line[0][eff_x[i]];
				}
			}

			if (!layer_line_empty[1]) {
				for (int i = 0; i < LAYER_PIXELS_PER_ITERATION; ++i) {
					l2_col_index[i] = layer_line[1][eff_x[i]];
				}
			}

			for (int i = 0; i < LAYER_PIXELS_PER_ITERATION; ++i) {
				spr_zindex[i] = sprite_line_z[eff_x[i]];
			}

			bool same_sprite = true;
			for (int i = 1; same_sprite && i < LAYER_PIXELS_PER_ITERATION; ++i) {
				same_sprite &= spr_zindex[0] == spr_zindex[i];
			}

			if (same_sprite) {
				switch (spr_zindex[0]) {
					case 3:
						for (int i = 0; i < LAYER_PIXELS_PER_ITERATION; ++i) {
							col_index[i] = spr_col_index[i] ?: l2_col_index[i] ?: l1_col_index[i];
						}
						break;
					case 2:
						for (int i = 0; i < LAYER_PIXELS_PER_ITERATION; ++i) {
							col_index[i] = l2_col_index[i] ?: spr_col_index[i] ?: l1_col_index[i];
						}
						break;
					case 1:
						for (int i = 0; i < LAYER_PIXELS_PER_ITERATION; ++i) {
							col_index[i] = l2_col_index[i] ?: l1_col_index[i] ?: spr_col_index[i];
						}
						break;
					case 0:
						for (int i = 0; i < LAYER_PIXELS_PER_ITERATION; ++i) {
							col_index[i] = l2_col_index[i] ?: l1_col_index[i];
						}
						break;
				}
			} else {
				for (int i = 0; i < LAYER_PIXELS_PER_ITERATION; ++i) {
					col_index[i] = calculate_line_col_index(spr_zindex[i], spr_col_index[i], l1_col_index[i], l2_col_index[i]);
				}
			}

			for (int i = 0; i < LAYER_PIXELS_PER_ITERATION; ++i) {
				col_line[x+i] = col_index[i];
			}
		}

		// Add border after if required.
		if (hstart > 0 || hstop < SCREEN_WIDTH || vstart > 0 || vstop < SCREEN_HEIGHT) {
			for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
				if (x < hstart || x > hstop || y < vstart || y > vstop) {
					col_line[x] = border_color;
				}
			}
		}
	}

	// Look up all color indices.
	uint32_t* framebuffer4_begin = ((uint32_t*)framebuffer) + (y * SCREEN_WIDTH);
	{
		uint32_t* framebuffer4 = framebuffer4_begin;
		for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
			*framebuffer4++ = video_palette.entries[col_line[x]];
		}
	}

	// NTSC overscan
	if (out_mode == 2) {
		uint32_t* framebuffer4 = framebuffer4_begin;
		for (uint16_t x = 0; x < SCREEN_WIDTH; x++)
		{
			if (x < SCREEN_WIDTH * TITLE_SAFE_X ||
				x > SCREEN_WIDTH * (1 - TITLE_SAFE_X) ||
				y < SCREEN_HEIGHT * TITLE_SAFE_Y ||
				y > SCREEN_HEIGHT * (1 - TITLE_SAFE_Y)) {

				// Divide RGB elements by 4.
				*framebuffer4 &= 0x00fcfcfc;
				*framebuffer4 >>= 2;
			}
			framebuffer4++;
		}
	}
}

bool
video_step(float mhz)
{
	uint8_t out_mode = reg_composer[0] & 3;

	bool new_frame = false;
	float advance = ((out_mode & 2) ? NTSC_PIXEL_FREQ :  VGA_PIXEL_FREQ) / mhz;
	scan_pos_x += advance;
	if (scan_pos_x > SCAN_WIDTH) {
		scan_pos_x -= SCAN_WIDTH;
		uint16_t front_porch = (out_mode & 2) ? NTSC_FRONT_PORCH_Y : VGA_FRONT_PORCH_Y;
		uint16_t y = scan_pos_y - front_porch;
		if (y < SCREEN_HEIGHT) {
			render_line(y);
		}
		scan_pos_y++;
		if (scan_pos_y == SCAN_HEIGHT) {
			scan_pos_y = 0;
			new_frame = true;
			if (ien & 1) { // VSYNC IRQ
				isr |= 1;
			}
		}
		if (ien & 2) { // LINE IRQ
			y = scan_pos_y - front_porch;
			uint16_t irq_line = reg_composer[9] | (reg_composer[10] & 1) << 8;
			if (y < SCREEN_HEIGHT && y == irq_line) {
				isr |= 2;
			}
		}
	}

	return new_frame;
}

bool
video_get_irq_out()
{
	return isr > 0;
}

//
// saves the video memory and register content into a file
//

void
video_save(FILE *f)
{
	fwrite(&video_ram[0], sizeof(uint8_t), sizeof(video_ram), f);
	fwrite(&reg_composer[0], sizeof(uint8_t), sizeof(reg_composer), f);
	fwrite(&palette[0], sizeof(uint8_t), sizeof(palette), f);
	fwrite(&reg_layer[0][0], sizeof(uint8_t), sizeof(reg_layer), f);
	fwrite(&reg_sprites[0], sizeof(uint8_t), sizeof(reg_sprites), f);
	fwrite(&sprite_data[0], sizeof(uint8_t), sizeof(sprite_data), f);
}

bool
video_update()
{
	static bool cmd_down = false;

	SDL_UpdateTexture(sdlTexture, NULL, framebuffer, SCREEN_WIDTH * 4);

	if (record_gif > RECORD_GIF_PAUSED) {
		if(!GifWriteFrame(&gif_writer, framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, 2, 8, false)) {
			// if that failed, stop recording
			GifEnd(&gif_writer);
			record_gif = RECORD_GIF_DISABLED;
			printf("Unexpected end of recording.\n");
		}
		if (record_gif == RECORD_GIF_SINGLE) { // if single-shot stop recording
			record_gif = RECORD_GIF_PAUSED;  // need to close in video_end()
		}
	}

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);

	if (debugger_enabled && showDebugOnRender != 0) {
		DEBUGRenderDisplay(SCREEN_WIDTH, SCREEN_HEIGHT);
		SDL_RenderPresent(renderer);
		return true;
	}

	SDL_RenderPresent(renderer);

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			return false;
		}
		if (event.type == SDL_KEYDOWN) {
			bool consumed = false;
			if (cmd_down) {
				if (event.key.keysym.sym == SDLK_s) {
					machine_dump();
					consumed = true;
				} else if (event.key.keysym.sym == SDLK_r) {
					machine_reset();
					consumed = true;
				} else if (event.key.keysym.sym == SDLK_v) {
					machine_paste(SDL_GetClipboardText());
					consumed = true;
				} else if (event.key.keysym.sym == SDLK_f || event.key.keysym.sym == SDLK_RETURN) {
					is_fullscreen = !is_fullscreen;
					SDL_SetWindowFullscreen(window, is_fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
					consumed = true;
				}
			}
			if (!consumed) {
				if (event.key.keysym.scancode == LSHORTCUT_KEY || event.key.keysym.scancode == RSHORTCUT_KEY) {
					cmd_down = true;
				}
				handle_keyboard(true, event.key.keysym.sym, event.key.keysym.scancode);
			}
			return true;
		}
		if (event.type == SDL_KEYUP) {
			if (event.key.keysym.scancode == LSHORTCUT_KEY || event.key.keysym.scancode == RSHORTCUT_KEY) {
				cmd_down = false;
			}
			handle_keyboard(false, event.key.keysym.sym, event.key.keysym.scancode);
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
			static int mouse_x;
			static int mouse_y;
			mouse_move(event.motion.x - mouse_x, event.motion.y - mouse_y);
			mouse_x = event.motion.x;
			mouse_y = event.motion.y;
		}
#if __APPLE__ && (TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE)
		if (event.type == SDL_DROPFILE) {	   // In case if dropped file
			char* dropped_filedir;
			dropped_filedir = event.drop.file;
			receiveFile(dropped_filedir);
			SDL_free(dropped_filedir);	  // Free dropped_filedir memory
		}
#endif
	}
	return true;
}

void
video_end()
{
	if (debugger_enabled) {
		DEBUGFreeUI();
	}

	if (record_gif != RECORD_GIF_DISABLED) {
		GifEnd(&gif_writer);
		record_gif = RECORD_GIF_DISABLED;
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}


uint32_t
get_and_inc_address(uint8_t sel)
{
	uint32_t address = io_addr[sel];
	if (io_inc[sel]) {
		io_addr[sel] += 1 << (io_inc[sel] - 1);
	}
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
	} else if (address >= ADDR_UART_START && address < ADDR_UART_END) {
		return vera_uart_read(address & 3);
	} else {
		return 0xFF; // unassigned
	}
}

static void
video_space_read_range(uint8_t* dest, uint32_t address, uint32_t size)
{
	if (address >= ADDR_VRAM_START && (address+size) <= ADDR_VRAM_END) {
		memcpy(dest, &video_ram[address], size);
	} else {
		for(int i = 0; i < size; ++i) {
			*dest++ = video_space_read(address + i);
		}
	}
}

void
video_space_write(uint32_t address, uint8_t value)
{
	if (address >= ADDR_VRAM_START && address < ADDR_VRAM_END) {
		video_ram[address] = value;
	} else if (address >= ADDR_LAYER1_START && address < ADDR_LAYER1_END) {
		reg_layer[0][address & 0xf] = value;
		refresh_layer_properties(0);
	} else if (address >= ADDR_LAYER2_START && address < ADDR_LAYER2_END) {
		reg_layer[1][address & 0xf] = value;
		refresh_layer_properties(1);
	} else if (address >= ADDR_SPRITES_START && address < ADDR_SPRITES_END) {
		reg_sprites[address & 0xf] = value;
	} else if (address >= ADDR_COMPOSER_START && address < ADDR_COMPOSER_END) {
		reg_composer[address & 0x1f] = value;
		video_palette.dirty = true;
	} else if (address >= ADDR_PALETTE_START && address < ADDR_PALETTE_END) {
		palette[address & 0x1ff] = value;
		video_palette.dirty = true;
	} else if (address >= ADDR_SPRDATA_START && address < ADDR_SPRDATA_END) {
		sprite_data[(address >> 3) & 0xff][address & 0x7] = value;
		refresh_sprite_properties((address >> 3) & 0xff);
	} else if (address >= ADDR_SPI_START && address < ADDR_SPI_END) {
		vera_spi_write(address & 1, value);
	} else if (address >= ADDR_UART_START && address < ADDR_UART_END) {
		vera_uart_write(address & 1, value);
	} else {
		// unassigned, do nothing
	}
}

//
// Vera: 6502 I/O Interface
//
// if debugOn, read without any side effects (registers & memory unchanged)

uint8_t
video_read(uint8_t reg, bool debugOn)
{
	switch (reg) {
		case 0:
			return io_addr[io_addrsel] & 0xff;
		case 1:
			return (io_addr[io_addrsel] >> 8) & 0xff;
		case 2:
			return (io_addr[io_addrsel] >> 16) | (io_inc[io_addrsel] << 4);
		case 3:
		case 4: {
			uint32_t address = debugOn ? io_addr[reg - 3] : get_and_inc_address(reg - 3);
			uint8_t value = video_space_read(address);
			if (log_video) {
				printf("READ  video_space[$%X] = $%02X\n", address, value);
			}
			return value;
		}
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

void
video_write(uint8_t reg, uint8_t value)
{
//	printf("ioregisters[%d] = $%02X\n", reg, value);
	switch (reg) {
		case 0:
			io_addr[io_addrsel] = (io_addr[io_addrsel] & 0xfff00) | value;
			break;
		case 2:
			io_addr[io_addrsel] = (io_addr[io_addrsel] & 0x0ffff) | ((value & 0xf) << 16);
			io_inc[io_addrsel] = value >> 4;
			break;
		case 1:
			io_addr[io_addrsel] = (io_addr[io_addrsel] & 0xf00ff) | (value << 8);
			break;
		case 3:
		case 4: {
			uint32_t address = get_and_inc_address(reg - 3);
			if (log_video) {
				printf("WRITE video_space[$%X] = $%02X\n", address, value);
			}
			video_space_write(address, value);
			break;
		}
		case 5:
			if (value & 0x80) {
				video_reset();
			}
			io_addrsel = value & 1;
			break;
		case 6:
			ien = value;
			break;
		case 7:
			isr &= value ^ 0xff;
			break;
	}
}

void
video_update_title(const char* window_title)
{
	SDL_SetWindowTitle(window, window_title);
}
