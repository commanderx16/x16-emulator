// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _GLUE_H_
#define _GLUE_H_

#include <stdint.h>
#include <stdbool.h>

//#define FIXED_KERNAL
//#define TRACE
#define LOAD_HYPERCALLS

#define NUM_RAM_BANKS 256
#define NUM_ROM_BANKS 8

#define RAM_SIZE (0xa000 + NUM_RAM_BANKS * 8192) /* $0000-$9FFF + banks at $A000-$BFFF */
#ifdef FIXED_KERNAL
#define ROM_SIZE (8192 + NUM_ROM_BANKS * 8192)   /* $E000-$FFFF + banks at $A000-$BFFF */
#else
#define ROM_SIZE (NUM_ROM_BANKS * 16384)   /* banks at $C000-$FFFF */
#endif

typedef enum {
	ECHO_MODE_NONE,
	ECHO_MODE_RAW,
	ECHO_MODE_COOKED,
} echo_mode_t;

extern uint8_t a, x, y, sp, status;
extern uint16_t pc;
extern uint8_t RAM[];
extern uint8_t ROM[];

extern bool debuger_enabled;
extern bool log_video;
extern bool log_keyboard;
echo_mode_t echo_mode;
extern bool save_on_exit;
extern bool record_gif;
extern char *gif_path;
extern uint8_t keymap;

extern void machine_dump();
extern void machine_reset();
extern void machine_paste();
extern void init_audio();

#endif
