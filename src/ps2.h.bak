// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef PS2_H
#define PS2_H

#include <stdint.h>

#define PS2_DATA_MASK (0x01)
#define PS2_CLK_MASK (0x02)
#define PS2_VIA_MASK (0x03)

typedef struct {
	uint8_t out;
	uint8_t in;
} ps2_port_t;

extern ps2_port_t ps2_port[2];

void ps2_buffer_add(int i, uint8_t byte);
void ps2_step(int i, int clocks);
void ps2_autostep(int i);

// fake mouse
void mouse_button_down(int num);
void mouse_button_up(int num);
void mouse_move(int x, int y);
uint8_t mouse_read(uint8_t reg);
void mouse_send_state(void);

#endif
