#include "video.h"
#include "memory.h"
#include "glue.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define SCREEN_RAM_OFFSET 0x00000

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *sdlTexture;

static uint8_t video_ram[0x20000];
static uint8_t chargen_rom[4096];
static uint8_t palette[256 * 2];

static uint8_t registers[8];
static uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT * 4];

#define KBD_BUFFER_SIZE 32
uint8_t kbd_buffer[KBD_BUFFER_SIZE];
uint8_t kbd_buffer_read = 0;
uint8_t kbd_buffer_write = 0;

static const uint8_t c64_palette[] = { // colodore
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
video_init(uint8_t *in_chargen)
{
	// init registers
	memset(registers, 0, sizeof(registers));

	// copy chargen into video RAM
	memcpy(chargen_rom, in_chargen, sizeof(chargen_rom));

	// init palette
	for (int i = 0; i < 16; i++) {
		uint8_t r = c64_palette[i * 3 + 0] >> 4;
		uint8_t g = c64_palette[i * 3 + 1] >> 4;
		uint8_t b = c64_palette[i * 3 + 2] >> 4;
		uint16_t entry = r << 8 | g << 4 | b;
		palette[i * 2 + 0] = entry & 0xff;
		palette[i * 2 + 1] = entry >> 8;
	}

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);

	sdlTexture = SDL_CreateTexture(renderer,
				       SDL_PIXELFORMAT_RGB888,
				       SDL_TEXTUREACCESS_STREAMING,
				       SCREEN_WIDTH, SCREEN_HEIGHT);

	return true;
}

int
ps2_scancode_from_SDLKey(SDL_Keycode k)
{
	switch (k) {
		case SDLK_BACKSPACE:
			return 0x66;
		case SDLK_TAB:
			return 0xd;
		case SDLK_CLEAR:
			return 0;
		case SDLK_RETURN:
			return 0x5a;
		case SDLK_PAUSE:
			return 0;
		case SDLK_ESCAPE:
			return 0x76;
		case SDLK_SPACE:
			return 0x29;
		case SDLK_EXCLAIM:
			return 0x16;
		case SDLK_QUOTEDBL:
			return 0x52;
		case SDLK_HASH:
			return 0x26;
		case SDLK_DOLLAR:
			return 0x25;
		case SDLK_AMPERSAND:
			return 0x3d;
		case SDLK_QUOTE:
			return 0x52;
		case SDLK_LEFTPAREN:
			return 0x46;
		case SDLK_RIGHTPAREN:
			return 0x45;
		case SDLK_ASTERISK:
			return 0x3e;
		case SDLK_PLUS:
			return 0x55;
		case SDLK_COMMA:
			return 0x41;
		case SDLK_MINUS:
			return 0x4e;
		case SDLK_PERIOD:
			return 0x49;
		case SDLK_SLASH:
			return 0x4a;
		case SDLK_0:
			return 0x45;
		case SDLK_1:
			return 0x16;
		case SDLK_2:
			return 0x1e;
		case SDLK_3:
			return 0x26;
		case SDLK_4:
			return 0x25;
		case SDLK_5:
			return 0x2e;
		case SDLK_6:
			return 0x36;
		case SDLK_7:
			return 0x3d;
		case SDLK_8:
			return 0x3e;
		case SDLK_9:
			return 0x46;
		case SDLK_COLON:
			return 0x4c;
		case SDLK_SEMICOLON:
			return 0x4c;
		case SDLK_LESS:
			return 0x41;
		case SDLK_EQUALS:
			return 0x55;
		case SDLK_GREATER:
			return 0x49;
		case SDLK_QUESTION:
			return 0x4a;
		case SDLK_AT:
			return 0x1e;
		case SDLK_LEFTBRACKET:
			return 0x54;
		case SDLK_BACKSLASH:
			return 0x5d;
		case SDLK_RIGHTBRACKET:
			return 0x5b;
		case SDLK_CARET:
			return 0x36;
		case SDLK_UNDERSCORE:
			return 0x4e;
		case SDLK_BACKQUOTE:
			return 0xe;
		case SDLK_a:
			return 0x1c;
		case SDLK_b:
			return 0x32;
		case SDLK_c:
			return 0x21;
		case SDLK_d:
			return 0x23;
		case SDLK_e:
			return 0x24;
		case SDLK_f:
			return 0x2b;
		case SDLK_g:
			return 0x34;
		case SDLK_h:
			return 0x33;
		case SDLK_i:
			return 0x43;
		case SDLK_j:
			return 0x3B;
		case SDLK_k:
			return 0x42;
		case SDLK_l:
			return 0x4B;
		case SDLK_m:
			return 0x3A;
		case SDLK_n:
			return 0x31;
		case SDLK_o:
			return 0x44;
		case SDLK_p:
			return 0x4D;
		case SDLK_q:
			return 0x15;
		case SDLK_r:
			return 0x2D;
		case SDLK_s:
			return 0x1B;
		case SDLK_t:
			return 0x2C;
		case SDLK_u:
			return 0x3C;
		case SDLK_v:
			return 0x2A;
		case SDLK_w:
			return 0x1D;
		case SDLK_x:
			return 0x22;
		case SDLK_y:
			return 0x35;
		case SDLK_z:
			return 0x1A;
		case SDLK_DELETE:
			return 0;
		case SDLK_UP:
			return 0x75 | 0x80;
		case SDLK_DOWN:
			return 0x72 | 0x80;
		case SDLK_RIGHT:
			return 0x74 | 0x80;
		case SDLK_LEFT:
			return 0x6b | 0x80;
		case SDLK_INSERT:
			return 0;
		case SDLK_HOME:
			return 0x6c | 0x80;
		case SDLK_END:
			return 0;
		case SDLK_PAGEUP:
			return 0;
		case SDLK_PAGEDOWN:
			return 0;
		case SDLK_F1:
			return 0x05;
		case SDLK_F2:
			return 0x06;
		case SDLK_F3:
			return 0x04;
		case SDLK_F4:
			return 0x0c;
		case SDLK_F5:
			return 0x03;
		case SDLK_F6:
			return 0x0b;
		case SDLK_F7:
			return 0x83; // XXX the MSB clashes with the "extended" flag!
		case SDLK_F8:
			return 0x0a;
		case SDLK_F9:
			return 0;
		case SDLK_F10:
			return 0;
		case SDLK_F11:
			return 0;
		case SDLK_F12:
			return 0;
		case SDLK_F13:
			return 0;
		case SDLK_F14:
			return 0;
		case SDLK_F15:
			return 0;
		case SDLK_RSHIFT:
			return 0x59;
		case SDLK_LSHIFT:
			return 0x12;
		case SDLK_LCTRL:
			return 0x14;
		case SDLK_RCTRL:
			return 0x14 | 0x80;
		case SDLK_LALT:
			return 0x11;
//		case SDLK_LGUI: // Windows/Command
//			return 0x5b | 0x80;
		default:
			return 0;
	}
}

void
kbd_buffer_add(uint8_t code)
{
	if ((kbd_buffer_write + 1) % KBD_BUFFER_SIZE == kbd_buffer_read) {
		// buffer full
		return;
	}

	kbd_buffer[kbd_buffer_write] = code;
	kbd_buffer_write = (kbd_buffer_write + 1) % KBD_BUFFER_SIZE;
}

uint8_t
kbd_buffer_remove()
{
	if (kbd_buffer_read == kbd_buffer_write) {
		return 0; // empty
	} else {
		uint8_t code = kbd_buffer[kbd_buffer_read];
		kbd_buffer_read = (kbd_buffer_read + 1) % KBD_BUFFER_SIZE;
		return code;
	}
}

bool
video_update()
{
	for (int y = 0; y < SCREEN_HEIGHT; y++) {
		for (int x = 0; x < SCREEN_WIDTH; x++) {
			uint32_t addr = SCREEN_RAM_OFFSET + (y / 8 * SCREEN_WIDTH / 8 + x / 8) * 2;
			uint8_t ch = video_ram[addr];
			uint8_t col = video_ram[addr + 1];
			int xx = x % 8;
			int yy = y % 8;
			uint8_t s = chargen_rom[ch * 8 + yy];
			if (s & (1 << (7 - xx))) {
				col &= 15;
			} else {
				col >>= 4;
			}
			uint16_t entry = palette[col * 2] | palette[col * 2 + 1] << 8;
			int fbi = (y * SCREEN_WIDTH + x) * 4;
			framebuffer[fbi + 0] = (entry & 0xf) << 4;
			framebuffer[fbi + 1] = ((entry >> 4) & 0xf) << 4;
			framebuffer[fbi + 2] = ((entry >> 8) & 0xf) << 4;
		}
	}

	SDL_UpdateTexture(sdlTexture, NULL, framebuffer, SCREEN_WIDTH * 4);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, sdlTexture, NULL, NULL);
	SDL_RenderPresent(renderer);

	static bool cmd_down = false;
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			return false;
		}
		if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.sym == SDLK_LGUI) { // Windows/Command
				cmd_down = true;
			} else if (cmd_down && event.key.keysym.sym == SDLK_s) {
				memory_save();
			} else {
	//			printf("DOWN 0x%02x\n", event.key.keysym.sym);
				int scancode = ps2_scancode_from_SDLKey(event.key.keysym.sym);
				if (scancode & 0x80) {
					kbd_buffer_add(0xe0);
				}
				kbd_buffer_add(scancode & 0x7f);
			}
			return true;
		}
		if (event.type == SDL_KEYUP) {
			if (event.key.keysym.sym == SDLK_LGUI) { // Windows/Command
				cmd_down = false;
			} else {
	//			printf("UP   0x%02x\n", event.key.keysym.sym);
				kbd_buffer_add(0xf0); // BREAK
				kbd_buffer_add(ps2_scancode_from_SDLKey(event.key.keysym.sym));
			}
			return true;
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

//
// Vera: Layer 1 Registers
//

static uint8_t
video_reg_read(uint8_t reg)
{
	printf("XXX reading from video reg %d\n", reg);
	return 0;
}

void
video_reg_write(uint8_t reg, uint8_t value)
{
	printf("XXX writing to video reg %d\n", reg);
}

//
// Vera: Internal Video Address Space
//

static uint8_t
video_ram_read(uint32_t address)
{
	if (address < 0x20000) {
		return video_ram[address];
	} else if (address < 0x21000) {
		return chargen_rom[address & 0xfff];
	} else if (address < 0x40000) {
		return 0xFF; // unassigned
	} else if (address < 0x40010) {
		return video_reg_read(address & 0xf);
	} else if (address < 0x40200) {
		return 0xFF; // unassigned
	} else if (address < 0x40400) {
		return palette[address & 0x40200];
	} else {
		return 0xFF; // unassigned
	}
}

void
video_ram_write(uint32_t address, uint8_t value)
{
	if (address < 0x20000) {
		video_ram[address] = value;
	} else if (address < 0x21000) {
		// ROM, do nothing
	} else if (address < 0x40000) {
		// unassigned, do nothing
	} else if (address < 0x40010) {
		video_reg_write(address & 0xf, value);
	} else if (address < 0x40200) {
		// unassigned, do nothing
	} else if (address < 0x40400) {
		palette[address & 0x40200] = value;
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
	if (reg == 3) {
		uint32_t address = get_and_inc_address();
//		printf("READ  video_ram[$%x] = $%02x\n", address, video_ram[address]);
		return video_ram_read(address);
	} else {
		return registers[reg];
	}
}

void
video_write(uint8_t reg, uint8_t value)
{
//	printf("registers[%d] = $%02x\n", reg, value);
	if (reg == 3) {
		uint32_t address = get_and_inc_address();
//		printf("WRITE video_ram[$%x] = $%02x\n", address, value);
		video_ram_write(address, value);
	} else {
		registers[reg] = value;
	}
}

//
// VIA#1
//

uint8_t
via1_read(uint8_t reg)
{
	if (reg == 0) { // PORT B: fake scan code for now
		uint8_t code = kbd_buffer_remove();
		if (code) {
//			printf("VIA1PB: $%02x\n", code);
		}
		return code;
	} else {
		return 0;
	}
}

void
via1_write(uint8_t reg, uint8_t value)
{
	// TODO
}
