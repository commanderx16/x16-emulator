// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef __APPLE__
#define _XOPEN_SOURCE   600
#define _POSIX_C_SOURCE 1
#endif
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#ifdef __MINGW32__
#include <ctype.h>
#endif
#include "cpu/fake6502.h"
#include "debugger/disasm.h"
#include "memory.h"
#include "video.h"
#include "via.h"
#include "ps2.h"
#include "vera_spi.h"
#include "sdcard.h"
#include "loadsave.h"
#include "glue.h"
#include "debugger/debugger.h"
#include "utf8.h"
#include "joystick.h"
#include "utf8_encode.h"
#include "rom_symbols.h"
#include "ym2151.h"
#include "audio.h"
#include "version.h"
#include "iniparser/iniparser.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <pthread.h>
#endif

typedef struct {
	int audio_buffers;
	char rom_path[PATH_MAX];
	char *prg_path;
	char *bas_path;
	char *sdcard_path;
	char *audio_dev_name;
	bool run_geos;
	bool run_test;
	int test_number;
	int window_scale;
	bool run_after_load;
	char *scale_quality;
	bool dump_cpu;
	bool dump_ram;
	bool dump_bank;
	bool dump_vram;
} Parms;

void *emulator_loop(void *param);
void emscripten_main_loop(void);

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

#ifdef PERFSTAT
uint32_t stat[65536];
#endif

bool debugger_enabled = false;
char *paste_text = NULL;
char paste_text_data[65536];
bool pasting_bas = false;

uint16_t num_ram_banks = 64; // 512 KB default

bool log_video = false;
bool log_speed = false;
bool log_keyboard = false;
bool warp_mode = false;
echo_mode_t echo_mode;
bool save_on_exit = true;
gif_recorder_state_t record_gif = RECORD_GIF_DISABLED;
char *gif_path = NULL;
uint8_t keymap = 0; // KERNAL's default

int frames;
int32_t sdlTicks_base;
int32_t last_perf_update;
int32_t perf_frame_count;
char window_title[30];

#ifdef TRACE
bool trace_mode = false;
uint16_t trace_address = 0;
#endif

int instruction_counter;
SDL_RWops *prg_file ;
int prg_override_start = -1;

Parms parms;

#ifdef TRACE
#include "rom_labels.h"
char *
label_for_address(uint16_t address)
{
	uint16_t *addresses;
	char **labels;
	int count;
	switch (memory_get_rom_bank()) {
		case 0:
			addresses = addresses_bank0;
			labels = labels_bank0;
			count = sizeof(addresses_bank0) / sizeof(uint16_t);
			break;
		case 1:
			addresses = addresses_bank1;
			labels = labels_bank1;
			count = sizeof(addresses_bank1) / sizeof(uint16_t);
			break;
		case 2:
			addresses = addresses_bank2;
			labels = labels_bank2;
			count = sizeof(addresses_bank2) / sizeof(uint16_t);
			break;
		case 3:
			addresses = addresses_bank3;
			labels = labels_bank3;
			count = sizeof(addresses_bank3) / sizeof(uint16_t);
			break;
		case 4:
			addresses = addresses_bank4;
			labels = labels_bank4;
			count = sizeof(addresses_bank4) / sizeof(uint16_t);
			break;
		case 5:
			addresses = addresses_bank5;
			labels = labels_bank5;
			count = sizeof(addresses_bank5) / sizeof(uint16_t);
			break;
		case 6:
			addresses = addresses_bank6;
			labels = labels_bank6;
			count = sizeof(addresses_bank6) / sizeof(uint16_t);
			break;
#if 0
		case 7:
			addresses = addresses_bank7;
			labels = labels_bank7;
			count = sizeof(addresses_bank7) / sizeof(uint16_t);
			break;
#endif
		default:
			addresses = NULL;
			labels = NULL;
	}

	if (!addresses) {
		return NULL;
	}

	for (int i = 0; i < count; i++) {
		if (address == addresses[i]) {
			return labels[i];
		}
	}
	return NULL;
}
#endif

void
machine_dump()
{
	int index = 0;
	char filename[22];
	for (;;) {
		if (!index) {
			strcpy(filename, "dump.bin");
		} else {
			sprintf(filename, "dump-%i.bin", index);
		}
		if (access(filename, F_OK) == -1) {
			break;
		}
		index++;
	}
	SDL_RWops *f = SDL_RWFromFile(filename, "wb");
	if (!f) {
		printf("Cannot write to %s!\n", filename);
		return;
	}

	if (parms.dump_cpu) {
		SDL_RWwrite(f, &a, sizeof(uint8_t), 1);
		SDL_RWwrite(f, &x, sizeof(uint8_t), 1);
		SDL_RWwrite(f, &y, sizeof(uint8_t), 1);
		SDL_RWwrite(f, &sp, sizeof(uint8_t), 1);
		SDL_RWwrite(f, &status, sizeof(uint8_t), 1);
		SDL_RWwrite(f, &pc, sizeof(uint16_t), 1);
	}
	memory_save(f, parms.dump_ram, parms.dump_bank);

	if (parms.dump_vram) {
		video_save(f);
	}

	SDL_RWclose(f);
	printf("Dumped system to %s.\n", filename);
}

void
machine_reset()
{
	memory_reset();
	vera_spi_init();
	via1_init();
	via2_init();
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

void
timing_init() {
	frames = 0;
	sdlTicks_base = SDL_GetTicks();
	last_perf_update = 0;
	perf_frame_count = 0;
}

void
timing_update()
{
	frames++;
	int32_t sdlTicks = SDL_GetTicks() - sdlTicks_base;
	int32_t diff_time = 1000 * frames / 60 - sdlTicks;
	if (!warp_mode && diff_time > 0) {
		usleep(1000 * diff_time);
	}

	if (sdlTicks - last_perf_update > 5000) {
		int32_t frameCount = frames - perf_frame_count;
		int perf = frameCount / 3;

		if (perf < 100 || warp_mode) {
			sprintf(window_title, "Commander X16 (%d%%)", perf);
			video_update_title(window_title);
		} else {
			video_update_title("Commander X16");
		}

		perf_frame_count = frames;
		last_perf_update = sdlTicks;
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

void
machine_toggle_warp()
{
	warp_mode = !warp_mode;
	timing_init();
}

uint8_t
iso8859_15_from_unicode(uint32_t c)
{
	// line feed -> carriage return
	if (c == '\n') {
		return '\r';
	}

	// translate Unicode characters not part of Latin-1 but part of Latin-15
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

	// all other Unicode characters are also unsupported
	if (c >= 256) {
		return '?';
	}

	// everything else is Latin-15 already
	return c;
}

uint32_t
unicode_from_iso8859_15(uint8_t c)
{
	// translate Latin-15 characters not part of Latin-1
	switch (c) {
		case 0xa4:
			return 0x20ac; // '€'
		case 0xa6:
			return 0x160; // 'Š'
		case 0xa8:
			return 0x161; // 'š'
		case 0xb4:
			return 0x17d; // 'Ž'
		case 0xb8:
			return 0x17e; // 'ž'
		case 0xbc:
			return 0x152; // 'Œ'
		case 0xbd:
			return 0x153; // 'œ'
		case 0xbe:
			return 0x178; // 'Ÿ'
		default:
			return c;
	}
}

// converts the character to UTF-8 and prints it
static void
print_iso8859_15_char(char c)
{
	char utf8[5];
	utf8_encode(utf8, unicode_from_iso8859_15(c));
	printf("%s", utf8);
}

static bool
is_kernal()
{
	return read6502(0xfff6) == 'M' && // only for KERNAL
			read6502(0xfff7) == 'I' &&
			read6502(0xfff8) == 'S' &&
			read6502(0xfff9) == 'T';
}

static void
usage()
{
	printf(
	"\nCommander X16 Emulator r%s (%s)\n"
	"(C)2019,2020 Michael Steil et al.\n"
	"All rights reserved. License: 2-clause BSD\n\n"

	"Usage: x16emu [option] ...\n\n"

	"-rom <rom.bin>\n"
	"\tOverride KERNAL/BASIC/* ROM file.\n"
	"-ram <ramsize>\n"
	"\tSpecify banked RAM size in KB (8, 16, 32, ..., 2048).\n"
	"\tThe default is 512.\n"
	"-keymap <keymap>\n"
	"\tEnable a specific keyboard layout decode table.\n"
	"-sdcard <sdcard.img>\n"
	"\tSpecify SD card image (partition map + FAT32)\n"
	"-prg <app.prg>[,<load_addr>]\n"
	"\tLoad application from the local disk into RAM\n"
	"\t(.PRG file with 2 byte start address header)\n"
	"\tThe override load address is hex without a prefix.\n"
	"-bas <app.txt>\n"
	"\tInject a BASIC program in ASCII encoding through the\n"
	"\tkeyboard.\n"
	"-run\n"
	"\tStart the -prg/-bas program using RUN or SYS, depending\n"
	"\ton the load address.\n"
	"-geos\n"
	"\tLaunch GEOS at startup.\n"
	"-warp\n"
	"\tEnable warp mode, run emulator as fast as possible.\n"
	"-echo [{iso|raw}]\n"
	"\tPrint all KERNAL output to the host's stdout.\n"
	"\tBy default, everything but printable ASCII characters get\n"
	"\tescaped. \"iso\" will escape everything but non-printable\n"
	"\tISO-8859-1 characters and convert the output to UTF-8.\n"
	"\t\"raw\" will not do any substitutions.\n"
	"\tWith the BASIC statement \"LIST\", this can be used\n"
	"\tto detokenize a BASIC program.\n"
	"-log {K|S|V}...\n"
	"\tEnable logging of (K)eyboard, (S)peed, (V)ideo.\n"
	"\tMultiple characters are possible, e.g. -log KS\n"
	"-gif <file.gif>[,wait]\n"
	"\tRecord a gif for the video output.\n"
	"\tUse ,wait to start paused.\n"
	"\tPOKE $9FB5,2 to start recording.\n"
	"\tPOKE $9FB5,1 to capture a single frame.\n"
	"\tPOKE $9FB5,0 to pause.\n"
	"-scale {1|2|3|4}\n"
	"\tScale output to an integer multiple of 640x480\n"
	"-quality {nearest|linear|best}\n"
	"\tScaling algorithm quality\n"
	"-debug [<address>]\n"
	"\tEnable debugger. Optionally, set a breakpoint\n"
	"-dump {C|R|B|V}...\n"
	"\tConfigure system dump: (C)PU, (R)AM, (B)anked-RAM, (V)RAM\n"
	"\tMultiple characters are possible, e.g. -dump CV ; Default: RB\n"
	"-joy1 {NES | SNES}\n"
	"\tChoose what type of joystick to use, e.g. -joy1 SNES\n"
	"-joy2 {NES | SNES}\n"
	"\tChoose what type of joystick to use, e.g. -joy2 SNES\n"
	"-sound <output device>\n"
	"\tSet the output device used for audio emulation\n"
	"-abufs <number of audio buffers>\n"
	"\tSet the number of audio buffers used for playback. (default: 8)\n"
	"\tIncreasing this will reduce stutter on slower computers,\n"
	"\tbut will increase audio latency.\n"
#ifdef TRACE
	"-trace [<address>]\n"
	"\tPrint instruction trace. Optionally, a trigger address\n"
	"\tcan be specified.\n"
#endif
	"-version\n"
	"\tPrint additional version information the emulator and ROM.\n"
	"\n"
	, VER, VER_NAME);

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

int findKeymap(const char *keymapName) {
	if(keymapName) {
		for (int i = 0; i < sizeof(keymaps)/sizeof(*keymaps); i++) {
			if (!strcmp(keymapName, keymaps[i])) {
				return i;
			}
		}
	}
	return -1;
}

void processParms(int argc, char **argv, Parms *parms) {
	const char *rom_filename = "rom.bin";

	char *base_path = SDL_GetBasePath();

	argc--;
	argv++;

	//
	// set default values first
	//
	parms->audio_buffers= 8;
	parms->run_geos= false;
	parms->run_test= false;
	parms->test_number= 0;
	parms->prg_path= NULL;
	parms->bas_path= NULL;
	parms->sdcard_path= NULL;
	parms->audio_dev_name= NULL;
	parms->window_scale= 1;
	parms->run_after_load= false;
	parms->scale_quality= "best";
	parms->dump_cpu = false;
	parms->dump_ram = false;
	parms->dump_bank = false;
	parms->dump_vram = false;


	// This causes the emulator to load ROM data from the executable's directory when
	// no ROM file is specified on the command line.
	memcpy(parms->rom_path, base_path, strlen(base_path) + 1);
	strncpy(parms->rom_path + strlen(parms->rom_path), rom_filename, PATH_MAX - strlen(parms->rom_path));

	//
	// read parms values from ini file
	//
	dictionary *iniDict= iniparser_load("x16emu.ini");
	if(iniDict) {

		parms->audio_buffers= iniparser_getint(iniDict, "main:abufs", 8);
		parms->run_geos= 1 == iniparser_getboolean(iniDict, "main:geos", 0);
		parms->test_number= iniparser_getint(iniDict, "main:test", -1);
		parms->run_test= parms->test_number != -1;
		parms->window_scale= iniparser_getint(iniDict, "main:scale", 1);
		if(parms->window_scale>4)
			parms->window_scale= 1;

		parms->prg_path= (char *)iniparser_getstring(iniDict, "main:prg", NULL);
		parms->bas_path= (char *)iniparser_getstring(iniDict, "main:bas", NULL);
		parms->sdcard_path= (char *)iniparser_getstring(iniDict, "main:sdcard", NULL);
		parms->audio_dev_name= (char *)iniparser_getstring(iniDict, "main:sound", NULL);

		gif_path= (char *)iniparser_getstring(iniDict, "main:gif", NULL);

		log_keyboard= 1 == iniparser_getboolean(iniDict, "main:log_keyboard", 0);
		log_speed= 1 == iniparser_getboolean(iniDict, "main:log_speed", 0);
		log_video= 1 == iniparser_getboolean(iniDict, "main:log_video", 0);

		parms->dump_cpu= 1 == iniparser_getboolean(iniDict, "main:dump_cpu", 0);
		parms->dump_ram= 1 == iniparser_getboolean(iniDict, "main:dump_ram", 0);
		parms->dump_bank= 1 == iniparser_getboolean(iniDict, "main:dump_bank", 0);
		parms->dump_vram= 1 == iniparser_getboolean(iniDict, "main:dump_vram", 0);

		int ramsize= iniparser_getint(iniDict, "main:ram", num_ram_banks*8);
		if(ramsize % 8 == 0)
			num_ram_banks= ramsize / 8;

		char *path= (char *)iniparser_getstring(iniDict, "main:rom", NULL);
		if(path)
			strncpy(parms->rom_path, path, PATH_MAX-1);

		int km= findKeymap(iniparser_getstring(iniDict, "main:keymap", NULL));
		if(km >= 0)
			keymap = km;

		warp_mode= 1 == iniparser_getboolean(iniDict, "main:warp", 0);
		debugger_enabled= 1 == iniparser_getboolean(iniDict, "debugger:enabled", 0);

		char *echo= (char *)iniparser_getstring(iniDict, "main:echo", NULL);
		if(echo) {
			if(!strcmp("raw", echo))
				echo_mode = ECHO_MODE_RAW;
			else if(!strcmp("iso", echo))
				echo_mode = ECHO_MODE_ISO;
			else
				echo_mode = ECHO_MODE_COOKED;
		}

		char *joystick= (char *)iniparser_getstring(iniDict, "main:joy1", NULL);
		if(joystick) {
			if(!strcmp("NES", joystick))
				joy1_mode = NES;
			else if(!strcmp("SNES", joystick))
				joy1_mode = SNES;
		}

		joystick= (char *)iniparser_getstring(iniDict, "main:joy2", NULL);
		if(joystick) {
			if(!strcmp("NES", joystick))
				joy2_mode = NES;
			else if(!strcmp("SNES", joystick))
				joy2_mode = SNES;
		}

		char *quality= (char *)iniparser_getstring(iniDict, "main:quality", NULL);
		if(quality) {
			if( !strcmp("nearest", quality) ||
				!strcmp("linear", quality) ||
				!strcmp("best", quality)) {
				parms->scale_quality = quality;
			}
		}

		iniparser_freedict(iniDict);
	}

	//
	// get parms values from CLI - always override INI if set
	//
	while (argc > 0) {
		if (!strcmp(argv[0], "-rom")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			strncpy(parms->rom_path, argv[0], PATH_MAX-1);
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-ram")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			int kb = atoi(argv[0]);
			bool found = false;
			for (int cmp = 8; cmp <= 2048; cmp *= 2) {
				if (kb == cmp)  {
					found = true;
				}
			}
			if (!found) {
				usage();
			}
			num_ram_banks = kb /8;
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-keymap")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage_keymap();
			}
			int km= findKeymap(argv[0]);
			if(km >= 0)
				keymap = km;
			else {
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
			parms->prg_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-run")) {
			argc--;
			argv++;
			parms->run_after_load = true;
		} else if (!strcmp(argv[0], "-bas")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			parms->bas_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-geos")) {
			argc--;
			argv++;
			parms->run_geos = true;
		} else if (!strcmp(argv[0], "-test")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			parms->test_number = atoi(argv[0]);
			parms->run_test = true;
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-sdcard")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			parms->sdcard_path = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-warp")) {
			argc--;
			argv++;
			warp_mode = true;
		} else if (!strcmp(argv[0], "-echo")) {
			argc--;
			argv++;
			if (argc && argv[0][0] != '-') {
				if (!strcmp(argv[0], "raw")) {
					echo_mode = ECHO_MODE_RAW;
				} else if (!strcmp(argv[0], "iso")) {
						echo_mode = ECHO_MODE_ISO;
				} else {
					usage();
				}
				argc--;
				argv++;
			} else {
				echo_mode = ECHO_MODE_COOKED;
			}
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
		} else if (!strcmp(argv[0], "-dump")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			for (char *p = argv[0]; *p; p++) {
				switch (tolower(*p)) {
					case 'c':
						parms->dump_cpu = true;
						break;
					case 'r':
						parms->dump_ram = true;
						break;
					case 'b':
						parms->dump_bank = true;
						break;
					case 'v':
						parms->dump_vram = true;
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
			// set up for recording
			record_gif = RECORD_GIF_PAUSED;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			gif_path = argv[0];
			argv++;
			argc--;
		} else if (!strcmp(argv[0], "-debug")) {
			argc--;
			argv++;
			debugger_enabled = true;
			if (argc && argv[0][0] != '-') {
				DEBUGSetBreakPoint((uint16_t)strtol(argv[0], NULL, 16));
				argc--;
				argv++;
			}
		} else if (!strcmp(argv[0], "-joy1")) {
			argc--;
			argv++;
			if (!strcmp(argv[0], "NES")) {
				joy_mode[0] = NES;
				argc--;
				argv++;
			} else if (!strcmp(argv[0], "SNES")) {
				joy_mode[0] = SNES;
				argc--;
				argv++;
			}
		} else if (!strcmp(argv[0], "-joy2")){
			argc--;
			argv++;
			if (!strcmp(argv[0], "NES")){
				joy_mode[1] = NES;
				argc--;
				argv++;
			} else if (!strcmp(argv[0], "SNES")){
				joy_mode[1] = SNES;
				argc--;
				argv++;
			}
		} else if (!strcmp(argv[0], "-joy3")){
			argc--;
			argv++;
			if (!strcmp(argv[0], "NES")){
				joy_mode[2] = NES;
				argc--;
				argv++;
			} else if (!strcmp(argv[0], "SNES")){
				joy_mode[2] = SNES;
				argc--;
				argv++;
			}
		} else if (!strcmp(argv[0], "-joy4")){
			argc--;
			argv++;
			if (!strcmp(argv[0], "NES")){
				joy_mode[3] = NES;
				argc--;
				argv++;
			} else if (!strcmp(argv[0], "SNES")){
				joy_mode[3] = SNES;
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
					parms->window_scale = 1;
					break;
				case '2':
					parms->window_scale = 2;
					break;
				case '3':
					parms->window_scale = 3;
					break;
				case '4':
					parms->window_scale = 4;
					break;
				default:
					usage();
				}
			}
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-quality")) {
			argc--;
			argv++;
			if(!argc || argv[0][0] == '-') {
				usage();
			}
			if (!strcmp(argv[0], "nearest") ||
				!strcmp(argv[0], "linear") ||
				!strcmp(argv[0], "best")) {
				parms->scale_quality = argv[0];
			} else {
				usage();
			}
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-sound")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				audio_usage();
			}
			parms->audio_dev_name = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-abufs")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			parms->audio_buffers = (int)strtol(argv[0], NULL, 10);
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-version")){
			printf("%s", VER_INFO);
			argc--;
			argv++;
			exit(0);
		} else {
			usage();
		}
	}

}

int
main(int argc, char **argv)
{
	processParms(argc, argv, &parms);

	SDL_RWops *f = SDL_RWFromFile(parms.rom_path, "rb");
	if (!f) {
		printf("Cannot open %s!\n", parms.rom_path);
		exit(1);
	}
	size_t rom_size = SDL_RWread(f, ROM, ROM_SIZE, 1);
	(void)rom_size;
	SDL_RWclose(f);

	if (parms.sdcard_path) {
		sdcard_file = SDL_RWFromFile(parms.sdcard_path, "r+b");
		if (!sdcard_file) {
			printf("Cannot open %s!\n", parms.sdcard_path);
			exit(1);
		}
		sdcard_attach();
	}

	prg_override_start = -1;
	if (parms.prg_path) {
		char *comma = strchr(parms.prg_path, ',');
		if (comma) {
			prg_override_start = (uint16_t)strtol(comma + 1, NULL, 16);
			*comma = 0;
		}

		prg_file = SDL_RWFromFile(parms.prg_path, "rb");
		if (!prg_file) {
			printf("Cannot open %s!\n", parms.prg_path);
			exit(1);
		}
	}

	if (parms.bas_path) {
		SDL_RWops *bas_file = SDL_RWFromFile(parms.bas_path, "r");
		if (!bas_file) {
			printf("Cannot open %s!\n", parms.bas_path);
			exit(1);
		}
		paste_text = paste_text_data;
		size_t paste_size = SDL_RWread(bas_file, paste_text, 1, sizeof(paste_text_data) - 1);
		if (parms.run_after_load) {
			strncpy(paste_text + paste_size, "\rRUN\r", sizeof(paste_text_data) - paste_size);
		} else {
			paste_text[paste_size] = 0;
		}
		SDL_RWclose(bas_file);
	}

	if (parms.run_geos) {
		paste_text = "GEOS\r";
	}
	if (parms.run_test) {
		paste_text = paste_text_data;
		snprintf(paste_text, sizeof(paste_text_data), "TEST %d\r", parms.test_number);
	}

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO);

	audio_init(parms.audio_dev_name, parms.audio_buffers);

	memory_init();
	video_init(parms.window_scale, parms.scale_quality);

	joystick_init();

	machine_reset();

	timing_init();

	instruction_counter = 0;

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(emscripten_main_loop, 0, 1);
#else
	emulator_loop(NULL);
#endif

	audio_close();
	video_end();
	SDL_Quit();

#ifdef PERFSTAT
	for (int pc = 0xc000; pc < sizeof(stat)/sizeof(*stat); pc++) {
		if (stat[pc] == 0) {
			continue;
		}
		char *label = label_for_address(pc);
		if (!label) {
			continue;
		}
		char *original_label = label;
		uint16_t pc2 = pc;
		if (label[0] == '@') {
			label = NULL;
			while (!label || label[0] == '@') {
				pc2--;
				label = label_for_address(pc2);
			}
		}
		printf("%d\t $%04X %s+%d", stat[pc], pc, label, pc-pc2);
		if (pc-pc2 != 0) {
			printf(" (%s)", original_label);
		}
		printf("\n");
	}
#endif

	return 0;
}

void
emscripten_main_loop(void) {
	emulator_loop(NULL);
}


void*
emulator_loop(void *param)
{
	for (;;) {

		if (debugger_enabled) {
			int dbgCmd = DEBUGGetCurrentStatus();
			if (dbgCmd > 0) continue;
			if (dbgCmd < 0) break;
		}

#ifdef PERFSTAT

//		if (memory_get_rom_bank() == 3) {
//			stat[pc]++;
//		}
		if (memory_get_rom_bank() == 3) {
			static uint8_t old_sp;
			static uint16_t base_pc;
			if (sp < old_sp) {
				base_pc = pc;
			}
			old_sp = sp;
			stat[base_pc]++;
		}
#endif

#ifdef TRACE
		if (pc == trace_address && trace_address != 0) {
			trace_mode = true;
		}
		if (trace_mode) {
			//printf("\t\t\t\t");
			printf("[%6d] ", instruction_counter);

			char *label = label_for_address(pc);
			int label_len = label ? strlen(label) : 0;
			if (label) {
				printf("%s", label);
			}
			for (int i = 0; i < 20 - label_len; i++) {
				printf(" ");
			}
			printf(" %02x:.,%04x ", memory_get_rom_bank(), pc);
			char disasm_line[15];
			int len = disasm(pc, RAM, disasm_line, sizeof(disasm_line), false, 0);
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

#if 0
			printf(" ---");
			for (int i = 0; i < 6; i++) {
				printf(" r%i:%04x", i, RAM[2 + i*2] | RAM[3 + i*2] << 8);
			}
			for (int i = 14; i < 16; i++) {
				printf(" r%i:%04x", i, RAM[2 + i*2] | RAM[3 + i*2] << 8);
			}

			printf(" RAM:%01x", memory_get_ram_bank());
			printf(" px:%d py:%d", RAM[0xa0e8] | RAM[0xa0e9] << 8, RAM[0xa0ea] | RAM[0xa0eb] << 8);

//			printf(" c:%d", RAM[0xa0e2]);
//			printf("-");
//			for (int i = 0; i < 10; i++) {
//				printf("%02x:", RAM[0xa041+i]);
//			}
#endif

			printf("\n");
		}
#endif

#ifdef LOAD_HYPERCALLS
		if ((pc == 0xffd5 || pc == 0xffd8) && is_kernal() && RAM[FA] == 8 && !sdcard_file) {
			if (pc == 0xffd5) {
				LOAD();
			} else {
				SAVE();
			}
			pc = (RAM[0x100 + sp + 1] | (RAM[0x100 + sp + 2] << 8)) + 1;
			sp += 2;
			continue;
		}
#endif

		uint32_t old_clockticks6502 = clockticks6502;
		step6502();
		uint8_t clocks = clockticks6502 - old_clockticks6502;
		bool new_frame = false;
		for (uint8_t i = 0; i < clocks; i++) {
			ps2_step(0);
			ps2_step(1);
			joystick_step();
			vera_spi_step();
			new_frame |= video_step(MHZ);
		}
		audio_render(clocks);

		instruction_counter++;

		if (new_frame) {
			if (!video_update()) {
				break;
			}

			timing_update();
#ifdef __EMSCRIPTEN__
			// After completing a frame we yield back control to the browser to stay responsive
			return 0;
#endif
		}

		if (video_get_irq_out()) {
			if (!(status & 4)) {
//				printf("IRQ!\n");
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
				machine_dump();
			}
			break;
		}

		if (echo_mode != ECHO_MODE_NONE && pc == 0xffd2 && is_kernal()) {
			uint8_t c = a;
			if (echo_mode == ECHO_MODE_COOKED) {
				if (c == 0x0d) {
					printf("\n");
				} else if (c == 0x0a) {
					// skip
				} else if (c < 0x20 || c >= 0x80) {
					printf("\\X%02X", c);
				} else {
					printf("%c", c);
				}
			} else if (echo_mode == ECHO_MODE_ISO) {
				if (c == 0x0d) {
					printf("\n");
				} else if (c == 0x0a) {
					// skip
				} else if (c < 0x20 || (c >= 0x80 && c < 0xa0)) {
					printf("\\X%02X", c);
				} else {
					print_iso8859_15_char(c);
				}
			} else {
				printf("%c", c);
			}
			fflush(stdout);
		}

		if (pc == 0xffcf && is_kernal()) {
			// as soon as BASIC starts reading a line...
			if (prg_file) {
				// ...inject the app into RAM
				uint8_t start_lo = SDL_ReadU8(prg_file);
				uint8_t start_hi = SDL_ReadU8(prg_file);
				uint16_t start;
				if (prg_override_start >= 0) {
					start = prg_override_start;
				} else {
					start = start_hi << 8 | start_lo;
				}
				uint16_t end = start + SDL_RWread(prg_file, RAM + start, 1, 65536-start);
				SDL_RWclose(prg_file);
				prg_file = NULL;
				if (start == 0x0801) {
					// set start of variables
					RAM[VARTAB] = end & 0xff;
					RAM[VARTAB + 1] = end >> 8;
				}

				if (parms.run_after_load) {
					if (start == 0x0801) {
						paste_text = "RUN\r";
					} else {
						paste_text = paste_text_data;
						snprintf(paste_text, sizeof(paste_text_data), "SYS$%04X\r", start);
					}
				}
			}

			if (paste_text) {
				// ...paste BASIC code into the keyboard buffer
				pasting_bas = true;
			}
		}

#if 0 // enable this for slow pasting
		if (!(instruction_counter % 100000))
#endif
		while (pasting_bas && RAM[NDX] < 10) {
			uint32_t c;
			int e = 0;

			if (paste_text[0] == '\\' && paste_text[1] == 'X' && paste_text[2] && paste_text[3]) {
				uint8_t hi = strtol((char[]){paste_text[2], 0}, NULL, 16);
				uint8_t lo = strtol((char[]){paste_text[3], 0}, NULL, 16);
				c = hi << 4 | lo;
				paste_text += 4;
			} else {
				paste_text = utf8_decode(paste_text, &c, &e);
				c = iso8859_15_from_unicode(c);
			}
			if (c && !e) {
				RAM[KEYD + RAM[NDX]] = c;
				RAM[NDX]++;
			} else {
				pasting_bas = false;
				paste_text = NULL;
			}
		}
	}

	return 0;
}
