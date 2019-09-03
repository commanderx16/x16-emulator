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
#include "fake6502.h"
#include "disasm.h"
#include "memory.h"
#include "video.h"
#include "ps2.h"
#include "sdcard.h"
#include "loadsave.h"
#include "glue.h"

#define MHZ 8

//#define TRACE
#define LOAD_HYPERCALLS

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
	printf("\tThe override load address is hex without a prefix.\n\n");
#ifdef TRACE
	printf("-trace [<address>]\n");
	printf("\tPrint instruction trace. Optionally, a trigger address\n");
	printf("\tcan be specified.\n");
#endif
	exit(1);
}
int
main(int argc, char **argv)
{
#ifdef TRACE
	bool trace = false;
	uint16_t trace_address = 0;
#endif

	char *rom_filename = "/rom.bin";
	char *char_filename = "/chargen.bin";
	char rom_path_data[PATH_MAX];
	char char_path_data[PATH_MAX];

	char *rom_path = rom_path_data;
	char *char_path = char_path_data;
	char *prg_path = NULL;
	char *sdcard_path = NULL;

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
		} else if (!strcmp(argv[0], "-sdcard")) {
			argc--;
			argv++;
			if (!argc) {
				usage();
			}
			sdcard_path = argv[0];
			argc--;
			argv++;
#ifdef TRACE

		} else if (!strcmp(argv[0], "-trace")) {
			argc--;
			argv++;
			if (argc && argv[0][0] != '-') {
				trace = false;
				trace_address = (uint16_t)strtol(argv[0], NULL, 16);
				argc--;
				argv++;
			} else {
				trace = true;
				trace_address = 0;
			}
#endif
		} else {
			usage();
		}
	}

	FILE *f = fopen(rom_path, "r");
	if (!f) {
		printf("Cannot open %s!\n", rom_path);
		exit(1);
	}
	int rom_size = fread(ROM, 1, ROM_SIZE, f);
	if (rom_size < 16384)  {
		printf("ROM needs to be at least 16 KB!\n");
		exit(1);
	}
	fclose(f);

	f = fopen(char_path, "r");
	if (!f) {
		printf("Cannot open %s!\n", char_path);
		exit(1);
	}
	uint8_t chargen[4096];
	int chargen_size = fread(chargen, 1, sizeof(chargen), f);
	if (chargen_size < 4096)  {
		printf("ROM needs to be at least 4 KB!\n");
		exit(1);
	}
	fclose(f);

	if (sdcard_path) {
		sdcard_file = fopen(sdcard_path, "r");
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

		prg_file = fopen(prg_path, "r");
		if (!prg_file) {
			printf("Cannot open %s!\n", prg_path);
			exit(1);
		}
	}

	video_init(chargen);
	sdcard_init();

	machine_reset();

	int instruction_counter = 0;
	for (;;) {
#ifdef TRACE
		if (pc == trace_address && trace_address != 0) {
			trace = true;
		}
		if (trace) {
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
			usleep(20000);
			if (!(status & 4)) {
				irq6502();
			}
		}

#if 0
		if (clockticks6502 >= 5 * MHZ * 1000 * 1000) {
			break;
		}
#endif

		if (pc == 0xffcf && is_kernal() && prg_file) {
			// as soon as BASIC starts reading a line,
			// inject the app
			uint8_t start_lo = fgetc(prg_file);
			uint8_t start_hi = fgetc(prg_file);
			uint16_t start;
			if (prg_override_start >= 0) {
				start = prg_override_start;
			} else {
				start = start_hi << 8 | start_lo;
			}
			int prg_size = fread(RAM + start, 1, 65536-start, prg_file);
			(void)prg_size; // make compiler happy
			fclose(prg_file);
			prg_file = NULL;
		}
	}

	video_end();

	return 0;
}
