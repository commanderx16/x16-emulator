// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _PS2_H_
#define _PS2_H_

#include <stdint.h>

#define PS2_DATA_MASK 1
#define PS2_CLK_MASK 2

typedef struct {
	int clk_out;
	int data_out;
	int clk_in;
	int data_in;
} ps2_port_t;

extern ps2_port_t ps2_port[2];

bool ps2_buffer_can_fit(int i, int n);
void ps2_buffer_add(int i, uint8_t byte);
void ps2_step(int i);

// fake mouse
void mouse_button_down(int num);
void mouse_button_up(int num);
void mouse_move(int x, int y);
uint8_t mouse_read(uint8_t reg);

#endif
