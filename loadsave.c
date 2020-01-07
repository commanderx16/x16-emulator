// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "glue.h"
#include "memory.h"
#include "video.h"
#include "rom_symbols.h"

#if __APPLE__
#include <TargetConditionals.h>
#endif

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

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

	#if __APPLE__ && (TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE)
	char filename[255];
	//use correct file location for ios
	strcpy(filename,getenv("HOME"));
	//concatenating the path string returned from HOME
	strcat(filename,"/Documents/.");
	#else
	const char *filename = ".";
	#endif

	if (!(dirp = opendir(filename))) {
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
LOAD()
{
	char filename[41];
	uint8_t len = MIN(RAM[FNLEN], sizeof(filename) - 1);
	memcpy(filename, (char *)&RAM[RAM[FNADR] | RAM[FNADR + 1] << 8], len);
	filename[len] = 0;

	uint16_t override_start = (x | (y << 8));

	if (filename[0] == '$') {
		uint16_t dir_len = create_directory_listing(RAM + override_start);
		uint16_t end = override_start + dir_len;
		x = end & 0xff;
		y = end >> 8;
		status &= 0xfe;
		RAM[STATUS] = 0;
		a = 0;
	} else {
		#if __APPLE__ && (TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE)
		//use correct file location for ios
		char full_filename[255];
		//concatenating the path string returned from HOME
		strcpy(full_filename, getenv("HOME"));
		strcat(full_filename, "/Documents/");
		strcat(full_filename, filename);
		#else
		char *full_filename = filename;
		#endif

		FILE *f = fopen(full_filename, "rb");

		if (!f) {
			a = 4; // FNF
			RAM[STATUS] = a;
			status |= 1;
			return;
		}
		uint8_t start_lo = fgetc(f);
		uint8_t start_hi = fgetc(f);
		uint16_t start;
		if (!RAM[SA]) {
			start = override_start;
		} else {
			start = start_hi << 8 | start_lo;
		}

		size_t bytes_read = 0;
		if(a > 1) {
			// Video RAM
			video_write(0, start & 0xff);
			video_write(1, start >> 8);
			video_write(2, ((a - 2) & 0xf) | 0x10);
			uint8_t buf[2048];
			while(1) {
				size_t n = fread(buf, 1, sizeof buf, f);
				if(n == 0) break;
				for(size_t i = 0; i < n; i++) {
					video_write(3, buf[i]);
				}
				bytes_read += n;
			}
		} else if(start < 0x9f00) {
			// Fixed RAM
			bytes_read = fread(RAM + start, 1, 0x9f00 - start, f);
		} else if(start < 0xa000) {
			// IO addresses
		} else if(start < 0xc000) {
			// banked RAM
			while(1) {
				size_t len = 0xc000 - start;
				bytes_read = fread(RAM + ((uint16_t)memory_get_ram_bank() << 13) + start, 1, len, f);
				if(bytes_read < len) break;

				// Wrap into the next bank
				start = 0xa000;
				memory_set_ram_bank(1 + memory_get_ram_bank());
			}
		} else {
			// ROM
		}

		fclose(f);

		uint16_t end = start + bytes_read;
		x = end & 0xff;
		y = end >> 8;
		status &= 0xfe;
		RAM[STATUS] = 0;
		a = 0;
	}
}

void
SAVE()
{
	char filename[41];
	uint8_t len = MIN(RAM[FNLEN], sizeof(filename) - 1);
	memcpy(filename, (char *)&RAM[RAM[FNADR] | RAM[FNADR + 1] << 8], len);
	filename[len] = 0;

	uint16_t start = RAM[a] | RAM[a + 1] << 8;
	uint16_t end = x | y << 8;
	if (end < start) {
		status |= 1;
		a = 0;
		return;
	}
	//use correct file location for ios
	#if __APPLE__ && (TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE)
	//use correct file location for ios
	char full_filename[255];
	//concatenating the path string returned from HOME
	strcpy(full_filename, getenv("HOME"));
	strcat(full_filename, "/Documents/");
	strcat(full_filename, filename);
	#else
	char *full_filename = filename;
	#endif

	FILE *f = fopen(full_filename, "wb");

	if (!f) {
		a = 4; // FNF
		RAM[STATUS] = a;
		status |= 1;
		return;
	}

	fputc(start & 0xff, f);
	fputc(start >> 8, f);

	fwrite(RAM + start, 1, end - start, f);
	fclose(f);

	status &= 0xfe;
	RAM[STATUS] = 0;
	a = 0;
}

