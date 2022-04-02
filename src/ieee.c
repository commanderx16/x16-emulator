// Commander X16 Emulator
// Copyright (c) 2022 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <SDL.h>
#include "rom_symbols.h"
#include "glue.h"

uint8_t name[80];
int namelen = 0;
char secaddr = 0;
bool listening = false;
bool talking = false;

uint8_t data[65536];
int data_len = 0;
int data_ptr = 0;

__attribute__((unused)) static void
set_z(char f)
{
	status = (status & ~2) | (!!f << 1);
}

static int
create_directory_listing(uint8_t *data)
{
	uint8_t *data_start = data;
	struct stat st;
	DIR *dirp;
	struct dirent *dp;
	int file_size;

	// We inject this directly into RAM, so
	// this does not include the load address!

	// start
	*data++ = 1;
	*data++ = 8;
	// link
	*data++ = 1;
	*data++ = 1;
	// line number
	*data++ = 0;
	*data++ = 0;
	*data++ = 0x12; // REVERSE ON
	*data++ = '"';
	for (int i = 0; i < 16; i++) {
		*data++ = ' ';
	}
	if (!(getcwd((char *)data - 16, 256))) {
		return false;
	}
	*data++ = '"';
	*data++ = ' ';
	*data++ = '0';
	*data++ = '0';
	*data++ = ' ';
	*data++ = 'P';
	*data++ = 'C';
	*data++ = 0;

	if (!(dirp = opendir("."))) {
		return 0;
	}
	while ((dp = readdir(dirp))) {
		size_t namlen = strlen(dp->d_name);
		stat(dp->d_name, &st);
		file_size = (st.st_size + 255)/256;
		if (file_size > 0xFFFF) {
			file_size = 0xFFFF;
		}

		// link
		*data++ = 1;
		*data++ = 1;

		*data++ = file_size & 0xFF;
		*data++ = file_size >> 8;
		if (file_size < 1000) {
			*data++ = ' ';
			if (file_size < 100) {
				*data++ = ' ';
				if (file_size < 10) {
					*data++ = ' ';
				}
			}
		}
		*data++ = '"';
		if (namlen > 16) {
			namlen = 16; // TODO hack
		}
		memcpy(data, dp->d_name, namlen);
		data += namlen;
		*data++ = '"';
		for (int i = namlen; i < 16; i++) {
			*data++ = ' ';
		}
		*data++ = ' ';
		*data++ = 'P';
		*data++ = 'R';
		*data++ = 'G';
		*data++ = 0;
	}

	// link
	*data++ = 1;
	*data++ = 1;

	*data++ = 255; // "65535"
	*data++ = 255;

	char *blocks_free = "BLOCKS FREE.";
	memcpy(data, blocks_free, strlen(blocks_free));
	data += strlen(blocks_free);
	*data++ = 0;

	// link
	*data++ = 0;
	*data++ = 0;
	(void)closedir(dirp);
	return data - data_start;
}

void
SECOND()
{
	printf("%s $%02x\n", __func__, a);
	if (listening) {
		secaddr = a;
		switch (secaddr & 0xf0) {
			case 0x60:
				printf("  WRITE %d...\n", a & 0xf);
				break;
			case 0xe0:
				printf("  CLOSE %d\n", a & 0xf);
				break;
			case 0xf0:
				printf("  OPEN %d...\n", a & 0xf);
				namelen = 0;
				break;
		}
	}
}

void
TKSA()
{
	printf("%s $%02x\n", __func__, a);
	if (talking) {
		secaddr = a;
	}
}

void
SETTMO()
{
	printf("%s\n", __func__);
}

void
ACPTR()
{
	if (data_ptr < data_len) {
		a = data[data_ptr++];
	}
	if (data_ptr == data_len) {
		RAM[STATUS] = 0x40;
	}
	set_z(!a);
	printf("%s-> $%02x\n", __func__, a);
}

void
CIOUT()
{
	printf("%s $%02x\n", __func__, a);
	if (listening) {
		if ((secaddr & 0xf0) == 0xf0) {
			if (namelen < sizeof(name)) {
				name[namelen++] = a;
			}
		} else {
			// write to file
		}
	}
}

void
UNTLK() {
	printf("%s\n", __func__);
	talking = false;
}

void
UNLSN() {
	printf("%s\n", __func__);
	listening = false;
	if ((secaddr & 0xf0) == 0xf0) {
		printf("  OPEN \"%s\",%d\n", name, secaddr & 0xf);
		data_len = create_directory_listing(data);
		data_ptr = 0;
	}
}

void
LISTEN()
{
	printf("%s $%02x\n", __func__, a);
	if (a == 8) {
		listening = true;
	}
}

void
TALK()
{
	printf("%s $%02x\n", __func__, a);
	if (a == 8) {
		talking = true;
	}
}
