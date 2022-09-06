// Commander X16 Emulator
// Copyright (c) 2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _I2C_H_
#define _I2C_H_

#include <stdint.h>

#define I2C_DATA_MASK 1
#define I2C_CLK_MASK 2

typedef struct {
	int clk_in;
	int data_in;
	int data_out;
} i2c_port_t;

extern i2c_port_t i2c_port;

void i2c_step();

void i2c_kbd_buffer_add(uint8_t value);
uint8_t i2c_kbd_buffer_next();
void i2c_kbd_buffer_flush();

void i2c_mse_buffer_add(uint8_t value);
uint8_t i2c_mse_buffer_next();
void i2c_mse_buffer_flush();
uint8_t i2c_mse_buffer_count();

// fake mouse
void mouse_button_down(int num);
void mouse_button_up(int num);
void mouse_move(int x, int y);
uint8_t mouse_read(uint8_t reg);
void mouse_send_state(void);

#endif
