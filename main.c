// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#define _XOPEN_SOURCE   600
#define _POSIX_C_SOURCE 1
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "cpu/fake6502.h"
#include "disasm.h"
#include "memory.h"
#include "video.h"
#include "via.h"
#include "ps2.h"
#include "spi.h"
#include "vera_spi.h"
#include "sdcard.h"
#include "loadsave.h"
#include "glue.h"
#include "debugger.h"
#include "utf8.h"
#ifdef WITH_YM2151
#include "ym2151.h"
#endif

#define AUDIO_SAMPLES 4096
#define SAMPLERATE 22050

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <pthread.h>
#endif

#define MHZ 8

//#define TRACE
#define LOAD_HYPERCALLS


void* emulator_loop(void *param);
void sdl_render_callback(void);


// This must match the KERNAL's set!
char *keymaps[] = {
	"en-us",
	"en-gb",
	"de",
	"nordic",
	"it",
	"pl",
	"hu",
	"es",
	"fr",
	"de-ch",
	"fr-be",
	"pt-br",
};

bool debuger_enabled = false;
char *paste_text = NULL;
char paste_text_data[65536];
bool pasting_bas = false;

bool log_video = false;
bool log_speed = false;
bool log_keyboard = false;
bool echo_mode = false;
bool save_on_exit = true;
bool record_gif = false;
char *gif_path = NULL;
uint8_t keymap = 0; // KERNAL's default
int window_scale = 1;


int instruction_counter;
FILE *prg_file ;
int prg_override_start = -1;
bool run_after_load = false;

#ifdef TRACE
#include "rom_labels.h"
char *
label_for_address(uint16_t address)
{
	for (int i = 0; i < sizeof(addresses) / sizeof(*addresses); i++) {
		if (address == addresses[i]) {
			return labels[i];
		}
	}
	return NULL;
}
#endif

void
machine_reset()
{
	video_reset();
	reset6502();
}

void
machine_paste(char *s)
{
	if (s) {
		paste_text = s;
		pasting_bas = true;
	}
}

uint8_t
latin15_from_unicode(uint32_t c)
{
	// line feed -> carriage return
	if (c == '\n') {
		return '\r';
	}

	// translate Unicode charaters not part of Latin-1 but part of Latin-15
	switch (c) {
		case 0x20ac: // '€'
			return 0xa4;
		case 0x160: // 'Š'
			return 0xa6;
		case 0x161: // 'š'
			return 0xa8;
		case 0x17d: // 'Ž'
			return 0xb4;
		case 0x17e: // 'ž'
			return 0xb8;
		case 0x152: // 'Œ'
			return 0xbc;
		case 0x153: // 'œ'
			return 0xbd;
		case 0x178: // 'Ÿ'
			return 0xbe;
	}

	// remove Unicode characters part of Latin-1 but not part of Latin-15
	switch (c) {
		case 0xa4: // '¤'
		case 0xa6: // '¦'
		case 0xa8: // '¨'
		case 0xb4: // '´'
		case 0xb8: // '¸'
		case 0xbc: // '¼'
		case 0xbd: // '½'
		case 0xbe: // '¾'
			return '?';
	}

	// all  other Unicode characters are also unsupported
	if (c >= 256) {
		return '?';
	}

	// everything else is Latin-15 already
	return c;
}

static bool
is_kernal()
{
	return ROM[0x3ff6] == 'M' && // only for KERNAL
	       ROM[0x3ff7] == 'I' &&
	       ROM[0x3ff8] == 'S' &&
	       ROM[0x3ff9] == 'T';
}

static void
usage()
{
	printf("\nCommander X16 Emulator  (C)2019 Michael Steil\n");
	printf("All rights reserved. License: 2-clause BSD\n\n");
	printf("Usage: x16emu [option] ...\n\n");
	printf("-rom <rom.bin>\n");
	printf("\tOverride KERNAL/BASIC/* ROM file:\n");
	printf("\t$0000-$1FFF bank #0 of banked ROM (BASIC)\n");
	printf("\t$2000-$3FFF fixed ROM at $E000-$FFFF (KERNAL)\n");
	printf("\t$4000-$5FFF bank #1 of banked ROM\n");
	printf("\t$6000-$7FFF bank #2 of banked ROM\n");
	printf("\t...\n");
	printf("\tThe file needs to be at least $4000 bytes in size.\n");
#ifndef VERA_V0_8
	printf("-char <chargen.bin>\n");
	printf("\tOverride character ROM file:\n");
	printf("\t$0000-$07FF upper case/graphics\n");
	printf("\t$0800-$0FFF lower case\n");
#endif
	printf("-keymap <keymap>\n");
	printf("\tEnable a specific keyboard layout decode table.\n");
	printf("-sdcard <sdcard.img>\n");
	printf("\tSpecify SD card image (partition map + FAT32)\n");
	printf("-prg <app.prg>[,<load_addr>]\n");
	printf("\tLoad application from the local disk into RAM\n");
	printf("\t(.PRG file with 2 byte start address header)\n");
	printf("\tThe override load address is hex without a prefix.\n");
	printf("-bas <app.txt>\n");
	printf("\tInject a BASIC program in ASCII encoding through the\n");
	printf("\tkeyboard.\n");
	printf("-run\n");
	printf("\tStart the -prg/-bas program using RUN or SYS, depending\n");
	printf("\ton the load address.\n");
	printf("-echo\n");
	printf("\tPrint all KERNAL output to the host's stdout.\n");
	printf("\tWith the BASIC statement \"LIST\", this can be used\n");
	printf("\tto detokenize a BASIC program.\n");
	printf("-log {K|S|V}...\n");
	printf("\tEnable logging of (K)eyboard, (S)peed, (V)ideo.\n");
	printf("\tMultiple characters are possible, e.g. -log KS\n");
	printf("-gif <file.gif>\n");
	printf("\tRecord a gif for the video output.\n");
	printf("-debug [<address>]\n");
	printf("\tEnable debugger. Optionally, set a breakpoint\n");
	printf("-scale {1|2|3|4}\n");
	printf("\tScale output to an integer multiple of 640x480\n");
	printf("\tEnable debugger.\n");
#ifdef TRACE
	printf("-trace [<address>]\n");
	printf("\tPrint instruction trace. Optionally, a trigger address\n");
	printf("\tcan be specified.\n");
#endif
	printf("\n");
	exit(1);
}

void
usage_keymap()
{
	printf("The following keymaps are supported:\n");
	for (int i = 0; i < sizeof(keymaps)/sizeof(*keymaps); i++) {
		printf("\t%s\n", keymaps[i]);
	}
	exit(1);
}

#ifdef WITH_YM2151
void audioCallback(void* userdata, Uint8 *stream, int len)
{
	YM_stream_update((uint16_t*) stream, len / 4);
}

void initAudio()
{
	SDL_AudioSpec want;
	SDL_AudioSpec have;

	// init YM2151 emulation. 4 MHz clock
	YM_Create(1.0f, 4000000);
	YM_init(SAMPLERATE, 60);

	// setup SDL audio
	want.freq = SAMPLERATE;
	want.format = AUDIO_S16;
	want.channels = 2;
	want.samples = AUDIO_SAMPLES;
	want.callback = audioCallback;
	want.userdata = NULL;
	if ( SDL_OpenAudio(&want, &have) < 0 ){
		fprintf(stderr, "SDL_OpenAudio failed: %s\n", SDL_GetError());
		exit(-1);
	}
	if (want.format != have.format || want.channels != have.channels) {
		// TODO: most soundcard should support signed 16 bit, but maybe add conversion functions
		printf("channels: %i, format: %i\n", have.format, have.channels);
		fprintf(stderr, "audio init failed\n");
		exit(-1);
	}

	// start playback
	SDL_PauseAudio(0);
}
#endif

int
main(int argc, char **argv)
{
#ifdef TRACE
	bool trace_mode = false;
	uint16_t trace_address = 0;
#endif

	char *rom_filename = "rom.bin";
#ifndef VERA_V0_8
	char *char_filename = "chargen.bin";
#endif
	char rom_path_data[PATH_MAX];
#ifndef VERA_V0_8
	char char_path_data[PATH_MAX];
#endif

	char *rom_path = rom_path_data;
#ifndef VERA_V0_8
	char *char_path = char_path_data;
#endif
	char *prg_path = NULL;
	char *bas_path = NULL;
	char *sdcard_path = NULL;

	run_after_load = false;

	char *base_path = SDL_GetBasePath();

	// This causes the emulator to load ROM data from the executable's directory when
	// no ROM file is specified on the command line.
	strncpy(rom_path, base_path, PATH_MAX);
	strncpy(rom_path + strlen(rom_path), rom_filename, PATH_MAX - strlen(rom_path));
#ifndef VERA_V0_8
	strncpy(char_path, base_path, PATH_MAX);
	strncpy(char_path + strlen(char_path), char_filename, PATH_MAX - strlen(char_path));
#endif

	argc--;
	argv++;

	while (argc > 0) {
		if (!strcmp(argv[0], "-rom")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			rom_path = argv[0];
			argc--;
			argv++;
#ifndef VERA_V0_8
		} else if (!strcmp(argv[0], "-char")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			char_path = argv[0];
			argc--;
			argv++;
#endif
		} else if (!strcmp(argv[0], "-keymap")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage_keymap();
			}
			bool found = false;
			for (int i = 0; i < sizeof(keymaps)/sizeof(*keymaps); i++) {
				if (!strcmp(argv[0], keymaps[i])) {
					found = true;
					keymap = i;
				}
			}
			if (!found) {
				usage_keymap();
			}
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-prg")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			prg_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-run")) {
			argc--;
			argv++;
			run_after_load = true;
		} else if (!strcmp(argv[0], "-bas")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			bas_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-sdcard")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			sdcard_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-echo")) {
			argc--;
			argv++;
			echo_mode = true;
		} else if (!strcmp(argv[0], "-log")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			for (char *p = argv[0]; *p; p++) {
				switch (tolower(*p)) {
					case 'k':
						log_keyboard = true;
						break;
					case 's':
						log_speed = true;
						break;
					case 'v':
						log_video = true;
						break;
					default:
						usage();
				}
			}
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-gif")) {
			argc--;
			argv++;
			record_gif = true;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			gif_path = argv[0];
			argv++;
			argc--;
		} else if (!strcmp(argv[0], "-debug")) {
			argc--;
			argv++;
			debuger_enabled = true;
			if (argc && argv[0][0] != '-') {
				DEBUGSetBreakPoint((uint16_t)strtol(argv[0], NULL, 16));
				argc--;
				argv++;
			}
#ifdef TRACE

		} else if (!strcmp(argv[0], "-trace")) {
			argc--;
			argv++;
			if (argc && argv[0][0] != '-') {
				trace_mode = false;
				trace_address = (uint16_t)strtol(argv[0], NULL, 16);
				argc--;
				argv++;
			} else {
				trace_mode = true;
				trace_address = 0;
			}
#endif
		} else if (!strcmp(argv[0], "-scale")) {
			argc--;
			argv++;
			if(!argc || argv[0][0] == '-') {
				usage();
			}
			for(char *p = argv[0]; *p; p++) {
				switch(tolower(*p)) {
				case '1':
					window_scale = 1;
					break;
				case '2':
					window_scale = 2;
					break;
				case '3':
					window_scale = 3;
					break;
				case '4':
					window_scale = 4;
					break;
				default:
					usage();
				}
			}
			argc--;
			argv++;
		} else {
			usage();
		}
	}

	FILE *f = fopen(rom_path, "rb");
	if (!f) {
		printf("Cannot open %s!\n", rom_path);
		exit(1);
	}
	int rom_size = fread(ROM, 1, ROM_SIZE, f);
	(void)rom_size;
	fclose(f);

#ifndef VERA_V0_8
	f = fopen(char_path, "rb");
	if (!f) {
		printf("Cannot open %s!\n", char_path);
		exit(1);
	}
	uint8_t chargen[4096];
	int chargen_size = fread(chargen, 1, sizeof(chargen), f);
	(void)chargen_size;
	fclose(f);
#endif

	if (sdcard_path) {
		sdcard_file = fopen(sdcard_path, "rb");
		if (!sdcard_file) {
			printf("Cannot open %s!\n", sdcard_path);
			exit(1);
		}
	}

	
	prg_override_start = -1;
	if (prg_path) {
		char *comma = strchr(prg_path, ',');
		if (comma) {
			prg_override_start = (uint16_t)strtol(comma + 1, NULL, 16);
			*comma = 0;
		}

		prg_file = fopen(prg_path, "rb");
		if (!prg_file) {
			printf("Cannot open %s!\n", prg_path);
			exit(1);
		}
	}

	if (bas_path) {
		FILE *bas_file = fopen(bas_path, "r");
		if (!bas_file) {
			printf("Cannot open %s!\n", bas_path);
			exit(1);
		}
		paste_text = paste_text_data;
		size_t paste_size = fread(paste_text, 1, sizeof(paste_text_data) - 1, bas_file);
		if (run_after_load) {
			strncpy(paste_text + paste_size, "\rRUN\r", sizeof(paste_text_data) - paste_size);
		} else {
			paste_text[paste_size] = 0;
		}
		fclose(bas_file);
	}

#ifdef WITH_YM2151
	initAudio();
#endif
	
#ifdef VERA_V0_8
	video_init(window_scale);
#else
	video_init(chargen, window_scale);
#endif
	spi_init();
	vera_spi_init();
	via1_init();
	via2_init();

	machine_reset();

	instruction_counter = 0;

#ifdef __EMSCRIPTEN__
	pthread_t tid;
    pthread_create(&tid, NULL, emulator_loop, NULL);
	emscripten_set_main_loop(sdl_render_callback, 60, 1);
#else
	emulator_loop(NULL);
#endif
	return 0;
}

void 
sdl_render_callback(void) {
	video_update();
}

void* 
emulator_loop(void *param)
	{
	for (;;) {

		if (debuger_enabled) {
			int dbgCmd = DEBUGGetCurrentStatus();
			if (dbgCmd > 0) continue;
			if (dbgCmd < 0) break;
		}

#ifdef TRACE
		if (pc == trace_address && trace_address != 0) {
			trace_mode = true;
		}
		if (trace_mode) {
			printf("\t\t\t\t[%6d] ", instruction_counter);

			char *label = label_for_address(pc);
			int label_len = label ? strlen(label) : 0;
			if (label) {
				printf("%s", label);
			}
			for (int i = 0; i < 10 - label_len; i++) {
				printf(" ");
			}
			printf(" .,%04x ", pc);
			char disasm_line[15];
			int len = disasm(pc, RAM, disasm_line, sizeof(disasm_line));
			for (int i = 0; i < len; i++) {
				printf("%02x ", read6502(pc + i));
			}
			for (int i = 0; i < 9 - 3 * len; i++) {
				printf(" ");
			}
			printf("%s", disasm_line);
			for (int i = 0; i < 15 - strlen(disasm_line); i++) {
				printf(" ");
			}

			printf("a=$%02x x=$%02x y=$%02x s=$%02x p=", a, x, y, sp);
			for (int i = 7; i >= 0; i--) {
				printf("%c", (status & (1 << i)) ? "czidb.vn"[i] : '-');
			}
//			printf(" --- %04x", RAM[0xae]  | RAM[0xaf]  << 8);
			printf("\n");
		}
#endif

#ifdef LOAD_HYPERCALLS
		if ((pc == 0xffd5 || pc == 0xffd8) && is_kernal() && RAM[0xba] == 1) {
			if (pc == 0xffd5) {
				LOAD();
			} else {
				SAVE();
			}
			pc = (RAM[0x100 + sp + 1] | (RAM[0x100 + sp + 2] << 8)) + 1;
			sp += 2;
		}
#endif

		uint32_t old_clockticks6502 = clockticks6502;
		step6502();
		uint8_t clocks = clockticks6502 - old_clockticks6502;
		bool new_frame = false;
		for (uint8_t i = 0; i < clocks; i++) {
			ps2_step();
			spi_step();
			vera_spi_step();
			new_frame |= video_step(MHZ);
		}

		instruction_counter++;

		if (new_frame) {
#ifndef __EMSCRIPTEN__
			if (!video_update()) {
				break;
			}
#endif

			static int frames = 0;
			frames++;
			int32_t diff_time = 1000 * frames / 60 - SDL_GetTicks();
			if (diff_time > 0) {
				usleep(1000 * diff_time);
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

		if (video_get_irq_out()) {
			if (!(status & 4)) {
				irq6502();
			}
		}

#if 0
		if (clockticks6502 >= 5 * MHZ * 1000 * 1000) {
			break;
		}
#endif

		if (pc == 0xffff) {
			if (save_on_exit) {
				memory_save();
			}
			break;
		}

		if (echo_mode && pc == 0xffd2 && is_kernal()) {
			uint8_t c = a;
			if (c == 13) {
				c = 10;
			}
			printf("%c", c);
		}

		if (pc == 0xffcf && is_kernal()) {
			// as soon as BASIC starts reading a line...
			if (prg_file) {
				// ...inject the app into RAM
				uint8_t start_lo = fgetc(prg_file);
				uint8_t start_hi = fgetc(prg_file);
				uint16_t start;
				if (prg_override_start >= 0) {
					start = prg_override_start;
				} else {
					start = start_hi << 8 | start_lo;
				}
				uint16_t end = start + fread(RAM + start, 1, 65536-start, prg_file);
				fclose(prg_file);
				prg_file = NULL;
				if (start == 0x0801) {
					// set start of variables
					RAM[0x2d] = end & 0xff;
					RAM[0x2e] = end >> 8;
				}

				if (run_after_load) {
					if (start == 0x0801) {
						paste_text = "RUN\r";
					} else {
						paste_text = paste_text_data;
						snprintf(paste_text, sizeof(paste_text_data), "SYS$%04x\r", start);
					}
				}
			}

			if (paste_text) {
				// ...paste BASIC code into the keyboard buffer
				pasting_bas = true;
			}
		}

		while (pasting_bas && RAM[0xc6] < 10) {
			uint32_t c;
			int e = 0;
			paste_text = utf8_decode(paste_text, &c, &e);

			c = latin15_from_unicode(c);
			if (c && !e) {
				RAM[0x0277 + RAM[0xc6]] = c;
				RAM[0xc6]++;
			} else {
				pasting_bas = false;
				paste_text = NULL;
			}
		}
	}

	video_end();

	return 0;
}

