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
#include "glue.h"

#define UNIT_NO 8

bool log_ieee = true;
//bool log_ieee = false;

char error[80];
int error_len = 0;
int error_pos = 0;
char cmd[80];
int cmdlen = 0;
int namelen = 0;
int channel = 0;
bool listening = false;
bool talking = false;
bool opening = false;

uint8_t dirlist[65536];
int dirlist_len = 0;
int dirlist_pos = 0;

typedef struct {
	char name[80];
	bool write;
	int pos;
	int size;
	SDL_RWops *f;
} channel_t;

channel_t channels[16];

#if 0
__attribute__((unused)) static void
set_z(char f)
{
	status = (status & ~2) | (!!f << 1);
}
#endif

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
	*data++ = 'H';
	*data++ = 'O';
	*data++ = 'S';
	*data++ = 'T';
	*data++ = ' ';
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

static char*
error_string(int e)
{
	switch(e) {
		case 0x00:
			return " OK";
		case 0x01:
			return " FILES SCRATCHED";
		case 0x02:
			return "PARTITION SELECTED";
		// 0x2x: Physical disk error
		case 0x20:
			return "READ ERROR"; // generic read error
		case 0x25:
			return "WRITE ERROR"; // generic write error
		case 0x26:
			return "WRITE PROTECT ON";
		// 0x3x: Error parsing the command
		case 0x30: // generic
		case 0x31: // invalid command
		case 0x32: // command buffer overflow
		case 0x33: // illegal filename
		case 0x34: // empty file name
		case 0x39: // subdirectory not found
			return "SYNTAX ERROR";
		// 0x4x: Controller error (CMD addition)
		case 0x49:
			return "INVALID FORMAT"; // partition present, but not FAT32
		// 0x5x: Relative file related error
		// unsupported
		// 0x6x: File error
		case 0x62:
			return " FILE NOT FOUND";
		case 0x63:
			return "FILE EXISTS";
		// 0x7x: Generic disk or device error
		case 0x70:
			return "NO CHANNEL"; // error allocating context
		case 0x71:
			return "DIRECTORY ERROR"; // FAT error
		case 0x72:
			return "PARTITION FULL"; // filesystem full
		case 0x73:
			return "HOST FS V1.0 X16";
		case 0x74:
			return "DRIVE NOT READY"; // illegal partition for any command but "CP"
		case 0x75:
			return "FORMAT ERROR";
		case 0x77:
			return "SELECTED PARTITION ILLEGAL";
		default:
			return "";
	}
}

static void
set_error(int e, int t, int s)
{
	snprintf(error, sizeof(error), "%02x,%s,%02d,%02d\r", e, error_string(e), t, s);
	error_len = strlen(error);
	error_pos = 0;
}

static void
clear_error()
{
	set_error(0, 0, 0);
}


static void
command(char *cmd)
{
	if (!cmd[0]) {
		return;
	}
	printf("  COMMAND \"%s\"\n", cmd);
	switch(cmd[0]) {
		case 'U':
			switch(cmd[1]) {
				case 'I': // UI: Reset
					set_error(0x73, 0, 0);
					return;
			}
		case 'I': // Initialize
			clear_error();
			return;
	}
	set_error(0x30, 0, 0);
}

static int
copen(int channel)
{
	if (channel == 15) {
		command(channels[channel].name);
		return -1;
	}

	int ret = -1;

	// decode ",P,W"-like suffix to know whether we're writing
	bool append = false;
	channels[channel].write = false;
	char *first = strchr(channels[channel].name, ',');
	if (first) {
		*first = 0; // truncate name here
		char *second = strchr(first+1, ',');
		if (second) {
			switch(second[1]) {
				case 'A':
					append = true;
					// fallthrough
				case 'W':
					channels[channel].write = true;
					break;
			}
		}
	}
	if (channel <= 1) {
		// channels 0 and 1 are magic
		channels[channel].write = channel;
	}
	if (log_ieee) {
		printf("  OPEN \"%s\",%d (%c)\n", channels[channel].name, channel, channels[channel].write ? 'W' : 'R');
	}

	if (!channels[channel].write && channels[channel].name[0] == '$') {
		dirlist_len = create_directory_listing(dirlist);
		dirlist_pos = 0;
	} else {
		if (!strcmp(channels[channel].name, ":*")) {
			channels[channel].f = prg_file;
		} else {
			channels[channel].f = SDL_RWFromFile(channels[channel].name, channels[channel].write ? "wb" : "rb");
		}
		if (!channels[channel].f) {
			if (log_ieee) {
				printf("  FILE NOT FOUND\n");
			}
			set_error(0x62, 0, 0);
#if 0
			a = 2; // FNF
			status |= 1;
			ret = a;
#endif

		} else {
			if (!channels[channel].write) {
				SDL_RWseek(channels[channel].f, 0, RW_SEEK_END);
				channels[channel].size = SDL_RWtell(channels[channel].f);
				SDL_RWseek(channels[channel].f, 0, RW_SEEK_SET);
				channels[channel].pos = 0;
			} else if (append) {
				SDL_RWseek(channels[channel].f, 0, RW_SEEK_END);
			}
			clear_error();
		}
	}
	return ret;
}

static void
cclose(int channel)
{
	if (log_ieee) {
		printf("  CLOSE %d\n", channel);
	}
	channels[channel].name[0] = 0;
	if (channels[channel].f) {
		SDL_RWclose(channels[channel].f);
		channels[channel].f = NULL;
	}
}

void
ieee_init()
{
	set_error(0x73, 0, 0);
}

void
SECOND(uint8_t a)
{
	if (log_ieee) {
		printf("%s $%02x\n", __func__, a);
	}
	if (listening) {
		channel = a & 0xf;
		opening = false;
		switch(a & 0xf0) {
			case 0x60:
				if (log_ieee) {
					printf("  WRITE %d...\n", channel);
				}
				break;
			case 0xe0:
				cclose(channel);
				break;
			case 0xf0:
				if (log_ieee) {
					printf("  OPEN %d...\n", channel);
				}
				opening = true;
				namelen = 0;
				break;
		}
	}
}

void
TKSA(uint8_t a)
{
	if (log_ieee) {
		printf("%s $%02x\n", __func__, a);
	}
	if (talking) {
		channel = a & 0xf;
	}
}


int
ACPTR(uint8_t *a)
{
	int ret = -1;
	if (channel == 15) {
		if (error_pos >= error_len) {
			clear_error();
		}
		*a = error[error_pos++];
	} else if (!channels[channel].write) {
		if (channels[channel].name[0] == '$') {
			if (dirlist_pos < dirlist_len) {
				*a = dirlist[dirlist_pos++];
			}
			if (dirlist_pos == dirlist_len) {
				ret = 0x40;
			}
		} else if (channels[channel].f) {
			*a = SDL_ReadU8(channels[channel].f);
			if (channels[channel].pos == channels[channel].size - 1) {
				ret = 0x40;
			} else {
				channels[channel].pos++;
			}
		}
	} else {
		ret = 2; // FNF
	}
#if 0
	set_z(!a);
#endif
	if (log_ieee) {
		printf("%s-> $%02x\n", __func__, *a);
	}
	return ret;
}

int
CIOUT(uint8_t a)
{
	int ret = -1;
	if (log_ieee) {
		printf("%s $%02x\n", __func__, a);
	}
	if (listening) {
		if (opening) {
			if (namelen < sizeof(channels[channel].name) - 1) {
				channels[channel].name[namelen++] = a;
			}
		} else {
			if (channel == 15) {
				if (a == 13) {
					cmd[cmdlen] = 0;
					command(cmd);
					cmdlen = 0;
				} else {
					if (cmdlen < sizeof(cmd) - 1) {
						cmd[cmdlen++] = a;
					}
				}
			} else if (channels[channel].write && channels[channel].f) {
				SDL_WriteU8(channels[channel].f, a);
			} else {
				ret = 2; // FNF
			}
		}
	}
	return ret;
}

void
UNTLK() {
	if (log_ieee) {
		printf("%s\n", __func__);
	}
	talking = false;
}

int
UNLSN() {
	int ret = -1;
	if (log_ieee) {
		printf("%s\n", __func__);
	}
	listening = false;
	if (opening) {
		channels[channel].name[namelen] = 0; // term
		opening = false;
		ret = copen(channel);
	} else if (channel == 15) {
		cmd[cmdlen] = 0;
		command(cmd);
		cmdlen = 0;
	}
	return ret;
}

void
LISTEN(uint8_t a)
{
	if (log_ieee) {
		printf("%s $%02x\n", __func__, a);
	}
	if ((a & 0x1f) == UNIT_NO) {
		listening = true;
	}
}

void
TALK(uint8_t a)
{
	if (log_ieee) {
		printf("%s $%02x\n", __func__, a);
	}
	if ((a & 0x1f) == UNIT_NO) {
		talking = true;
	}
}
