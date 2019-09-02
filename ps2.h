// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _PS2_H_
#define _PS2_H_

#include <stdint.h>

#define PS2_DATA_MASK 1
#define PS2_CLK_MASK 2

extern int ps2_clk_out, ps2_data_out;
extern int ps2_clk_in, ps2_data_in;

void kbd_buffer_add(uint8_t code);
uint8_t kbd_buffer_remove();
void ps2_step();

// fake mouse
void mouse_button_down(int num);
void mouse_button_up(int num);
void mouse_move(int x, int y);
uint8_t mouse_read(uint8_t reg);

#endif
