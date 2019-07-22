// Copyright (c) 2009 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "fake6502.h"
#include "disasm.h"
#include "video.h"
#include "glue.h"

#define MHZ 8

//#define DEBUG

#ifdef DEBUG
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

int
main(int argc, char **argv)
{
	if (argc < 3) {
		printf("Usage: %s <rom.bin> <chargen.bin> [<app.prg>]\n", argv[0]);
		exit(1);
	}

	FILE *f = fopen(argv[1], "r");
	if (!f) {
		printf("Cannot open %s!\n", argv[1]);
		exit(1);
	}

	uint8_t *rom = malloc(65536);
	size_t rom_size = fread(rom, 1, 65536, f);
	fclose(f);
	memcpy(RAM + 65536 - rom_size, rom, rom_size);
	free(rom);

	f = fopen(argv[2], "r");
	if (!f) {
		printf("Cannot open %s!\n", argv[2]);
		exit(1);
	}
	uint8_t chargen[4096];
	fread(chargen, 1, sizeof(chargen), f);
	fclose(f);

	if (argc == 4) {
		f = fopen(argv[3], "r");
		if (!f) {
			printf("Cannot open %s!\n", argv[3]);
			exit(1);
		}
		uint8_t start_lo = fgetc(f);
		uint8_t start_hi = fgetc(f);
		uint16_t start = start_hi << 8 | start_lo;
		fread(RAM + start, 1, 65536-start, f);
		fclose(f);
	}

	video_init(chargen);

	reset6502();

	int instruction_counter = 0;
	for (;;) {
#ifdef DEBUG
		printf("[%6d] ", instruction_counter);

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
			printf("%02x ", RAM[pc + i]);
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
		printf("\n");
#endif
//		if (pc == 0xffd2) {
//			printf("BSOUT: '%c'\n", a);
//		}

		step6502();
		instruction_counter++;

#if 0
		if (instruction_counter == 1778) {
			for (int i = 0; i < 25; i++) {
				printf("%02x ", RAM[0xd9 +i]);
			}
			printf("\n");
		}
#endif

		if (instruction_counter % (20000 * MHZ) == 0) {
			if (!video_update()) {
				break;
			}
			usleep(20000);
//			printf("IRQ!\n");
			irq6502();
		}
	}

	video_end();

	return 0;
}

