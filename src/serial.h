// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

typedef struct {
	int atn_out;  // VIA#1 PB3
	int clk_out;  // VIA#1 PB4
	int data_out; // VIA#1 PB5
	int clk_in;   // VIA#1 PB6
	int data_in;  // VIA#1 PB7
} serial_port_t;

extern serial_port_t serial_port;

void serial_step();

#endif
