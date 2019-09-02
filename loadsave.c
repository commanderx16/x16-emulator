// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <string.h>
#include <stdio.h>
#include "glue.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

void
LOAD()
{
	char filename[41];
	uint8_t len = MIN(RAM[0xb7], sizeof(filename) - 1);
	memcpy(filename, (char *)&RAM[RAM[0xbb] | RAM[0xbc] << 8], len);
	filename[len] = 0;

	uint16_t override_start = (x | (y << 8));

	FILE *f = fopen(filename, "r");
	if (!f) {
		a = 4; // FNF
		RAM[0x90] = a;
		status |= 1;
		return;
	}
	uint8_t start_lo = fgetc(f);
	uint8_t start_hi = fgetc(f);
	uint16_t start;
	if (!RAM[0xb9]) {
		start = override_start;
	} else {
		start = start_hi << 8 | start_lo;
	}

	size_t bytes_read = fread(RAM + start, 1, 0x9f00 - start, f);
	fclose(f);
	uint16_t end = start + bytes_read;

	x = end & 0xff;
	y = end >> 8;
	status &= 0xfe;
	RAM[0x90] = 0;
	a = 0;
}

void
SAVE()
{
	char filename[41];
	uint8_t len = MIN(RAM[0xb7], sizeof(filename) - 1);
	memcpy(filename, (char *)&RAM[RAM[0xbb] | RAM[0xbc] << 8], len);
	filename[len] = 0;

	uint16_t start = RAM[a] | RAM[a + 1] << 8;
	uint16_t end = x | y << 8;
	if (end < start) {
		status |= 1;
		a = 0;
		return;
	}

	FILE *f = fopen(filename, "w");
	if (!f) {
		a = 4; // FNF
		RAM[0x90] = a;
		status |= 1;
		return;
	}

	fputc(start & 0xff, f);
	fputc(start >> 8, f);

	fwrite(RAM + start, 1, end - start, f);
	fclose(f);

	status &= 0xfe;
	RAM[0x90] = 0;
	a = 0;
}

