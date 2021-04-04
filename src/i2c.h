// Commander X16 Emulator
// Copyright (c) 2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _I2C_H_
#define _I2C_H_

#include <stdint.h>

#define I2C_DATA_MASK 4

typedef struct {
	int clk_in;
	int data_in;
	int data_out;
} i2c_port_t;

extern i2c_port_t i2c_port;

void i2c_step();

#endif
