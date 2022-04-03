// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

#define SERIAL_ATNIN_MASK   (1<<3)
#define SERIAL_CLOCKIN_MASK (1<<4)
#define SERIAL_DATAIN_MASK  (1<<5)

typedef struct {
	int atn_in;
	int clk_in;
	int data_in;
	int clk_out;
	int data_out;
} serial_port_t;

extern serial_port_t serial_port;

void serial_step(int clocks);

#endif
