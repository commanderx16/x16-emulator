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
char channel = 0;
bool listening = false;
bool talking = false;
bool opening = false;

uint8_t dirlist[65536];
int dirlist_len = 0;
int dirlist_ptr = 0;

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

	// load address
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
		channel = a & 0xf;
		opening = false;
		switch (a & 0xf0) {
			case 0x60:
				printf("  WRITE %d...\n", channel);
				break;
			case 0xe0:
				printf("  CLOSE %d\n", channel);
				break;
			case 0xf0:
				printf("  OPEN %d...\n", channel);
				opening = true;
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
		channel = a & 0xf;
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
	if (dirlist_ptr < dirlist_len) {
		a = dirlist[dirlist_ptr++];
	}
	if (dirlist_ptr == dirlist_len) {
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
		if (opening) {
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
	if (opening) {
		printf("  OPEN \"%s\",%d\n", name, channel);
		dirlist_len = create_directory_listing(dirlist);
		dirlist_ptr = 0;
		opening = false;
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
