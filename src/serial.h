// Commander X16 Emulator
// Copyright (c) 2022 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

#define SERIAL_ATNIN_MASK   (1<<3)
#define SERIAL_CLOCKIN_MASK (1<<4)
#define SERIAL_DATAIN_MASK  (1<<5)

typedef struct {
	struct {
		int atn;
		int clk;
		int data;
	} in;
	struct {
		int clk;
		int data;
	} out;
} serial_port_t;

extern serial_port_t serial_port;

void serial_step(int clocks);

bool serial_port_read_clk();
bool serial_port_read_data();

#endif
