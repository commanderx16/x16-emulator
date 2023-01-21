// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _GLUE_H_
#define _GLUE_H_

#include <stdint.h>
#include <stdbool.h>
#include <SDL.h>

//#define TRACE
//#define PERFSTAT

#define MHZ 8

#define NUM_MAX_RAM_BANKS 256
#define NUM_ROM_BANKS 32

#define RAM_SIZE (0xa000 + num_ram_banks * 8192) /* $0000-$9FFF + banks at $A000-$BFFF */
#define ROM_SIZE (NUM_ROM_BANKS * 16384)   /* banks at $C000-$FFFF */

typedef enum {
	ECHO_MODE_NONE,
	ECHO_MODE_RAW,
	ECHO_MODE_COOKED,
	ECHO_MODE_ISO,
} echo_mode_t;

// GIF recorder commands
typedef enum {
	RECORD_GIF_PAUSE,
	RECORD_GIF_SNAP,
	RECORD_GIF_RESUME
} gif_recorder_command_t;

// GIF recorder states
typedef enum {
	RECORD_GIF_DISABLED,
	RECORD_GIF_PAUSED,
	RECORD_GIF_SINGLE,
	RECORD_GIF_ACTIVE
} gif_recorder_state_t;

extern uint8_t a, x, y, sp, status;
extern uint16_t pc;
extern uint8_t *RAM;
extern uint8_t ROM[];

extern uint16_t num_ram_banks;

extern bool debugger_enabled;
extern bool log_video;
extern bool log_keyboard;
extern bool log_speed;
extern echo_mode_t echo_mode;
extern bool save_on_exit;
extern bool disable_emu_cmd_keys;
extern gif_recorder_state_t record_gif;
extern char *gif_path;
extern char *fsroot_path;
extern char *startin_path;
extern uint8_t keymap;
extern bool warp_mode;
extern bool testbench;

extern void machine_dump(const char* reason);
extern void machine_reset();
extern void machine_paste(char *text);
extern void machine_toggle_warp();
extern void init_audio();

extern bool video_is_tilemap_address(int addr);
extern bool video_is_tiledata_address(int addr);
extern bool video_is_special_address(int addr);

extern uint8_t activity_led;
extern bool nvram_dirty;
extern uint8_t nvram[0x40];

#endif
