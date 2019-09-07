// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

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
#include "ps2.h"
#include "sdcard.h"
#include "loadsave.h"
#include "glue.h"
#include "debugger.h"

#define MHZ 8

//#define TRACE
#define LOAD_HYPERCALLS

bool debuger_enabled = false;
char *paste_text = NULL;
char paste_text_data[65536];
bool pasting_bas = false;

bool log_video = false;
bool log_speed = false;
bool log_keyboard = false;
bool echo_mode = false;
bool save_on_exit = true;

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
	printf("-char <chargen.bin>\n");
	printf("\tOverride character ROM file:\n");
	printf("\t$0000-$07FF upper case/graphics\n");
	printf("\t$0800-$0FFF lower case\n");
	printf("-sdcard <sdcard.img>\n");
	printf("\tSpecify SD card image (partition map + FAT32)\n");
	printf("-prg <app.prg>[,<load_addr>]\n");
	printf("\tLoad application from the local disk into RAM\n");
	printf("\t(.PRG file with 2 byte start address header)\n");
	printf("\tThe override load address is hex without a prefix.\n");
	printf("-run <app.prg>[,<load_addr>]\n");
	printf("\tSame as above, but also starts the application\n");
	printf("\tusing RUN or SYS, depending on the load address.\n");
	printf("-bas <app.txt>\n");
	printf("\tInject a BASIC program in ASCII encoding through the\n");
	printf("\tkeyboard.\n");
	printf("-echo\n");
	printf("\tPrint all KERNAL output to the host's stdout.\n");
	printf("\tWith the BASIC statement \"LIST\", this can be used\n");
	printf("\tto detokenize a BASIC program.\n");
	printf("-log {K|S|V}...\n");
	printf("\tEnable logging of (K)eyboard, (S)peed, (V)ideo.\n");
	printf("\tMultiple characters are possible, e.g. -log KS\n");
	printf("-debug\n");
	printf("\tEnable debugger.\n");
#ifdef TRACE
	printf("-trace [<address>]\n");
	printf("\tPrint instruction trace. Optionally, a trigger address\n");
	printf("\tcan be specified.\n");
#endif
	printf("\n");
	exit(1);
}
int
main(int argc, char **argv)
{
#ifdef TRACE
	bool trace_mode = false;
	uint16_t trace_address = 0;
#endif

	char *rom_filename = "/rom.bin";
	char *char_filename = "/chargen.bin";
	char rom_path_data[PATH_MAX];
	char char_path_data[PATH_MAX];

	char *rom_path = rom_path_data;
	char *char_path = char_path_data;
	char *prg_path = NULL;
	char *bas_path = NULL;
	char *sdcard_path = NULL;

	bool run_after_load = false;

#ifdef __APPLE__
	// on macOS, double clicking runs an executable in the user's
	// home directory, so we prepend the executable's path to
	// the rom filenames
	if (argv[0][0] == '/') {
		*strrchr(argv[0], '/') = 0;

		strncpy(rom_path, argv[0], PATH_MAX);
		strncpy(rom_path + strlen(rom_path), rom_filename, PATH_MAX - strlen(rom_path));
		strncpy(char_path, argv[0], PATH_MAX);
		strncpy(char_path + strlen(char_path), char_filename, PATH_MAX - strlen(char_path));
	} else
#endif
	{
		strncpy(rom_path, rom_filename + 1, PATH_MAX);
		strncpy(char_path, char_filename + 1, PATH_MAX);
	}

	argc--;
	argv++;

	while (argc > 0) {
		if (!strcmp(argv[0], "-rom")) {
			argc--;
			argv++;
			if (!argc) {
				usage();
			}
			rom_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-char")) {
			argc--;
			argv++;
			if (!argc) {
				usage();
			}
			char_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-prg")) {
			argc--;
			argv++;
			if (!argc) {
				usage();
			}
			prg_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-run")) {
			argc--;
			argv++;
			if (!argc) {
				usage();
			}
			prg_path = argv[0];
			run_after_load = true;
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-bas")) {
			argc--;
			argv++;
			if (!argc) {
				usage();
			}
			bas_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-sdcard")) {
			argc--;
			argv++;
			if (!argc) {
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
			if (!argc) {
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
		} else if (!strcmp(argv[0], "-debug")) {
			argc--;
			argv++;
			debuger_enabled = true;
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

	f = fopen(char_path, "rb");
	if (!f) {
		printf("Cannot open %s!\n", char_path);
		exit(1);
	}
	uint8_t chargen[4096];
	int chargen_size = fread(chargen, 1, sizeof(chargen), f);
	(void)chargen_size;
	fclose(f);

	if (sdcard_path) {
		sdcard_file = fopen(sdcard_path, "rb");
		if (!sdcard_file) {
			printf("Cannot open %s!\n", sdcard_path);
			exit(1);
		}
	}

	FILE *prg_file = NULL;
	int prg_override_start = -1;
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
		paste_text[paste_size] = 0;
		fclose(bas_file);
	}

	video_init(chargen);
	sdcard_init();

	machine_reset();

	int instruction_counter = 0;
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
			sdcard_step();
			new_frame |= video_step(MHZ);
		}

		instruction_counter++;

		if (new_frame) {
			if (!video_update()) {
				break;
			}

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
			uint8_t c = *paste_text;
			if (c) {
				if (c == 10) {
					c = 13;
				}
				RAM[0x0277 + RAM[0xc6]] = c;
				RAM[0xc6]++;
				paste_text++;
			} else {
				pasting_bas = false;
				paste_text = NULL;
			}
		}
	}

	video_end();

	return 0;
}
