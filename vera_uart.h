// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <inttypes.h>

extern FILE *uart_in_file;
extern FILE *uart_out_file;

void vera_uart_init();
void vera_uart_step();
uint8_t vera_uart_read(uint8_t address);
void vera_uart_write(uint8_t address, uint8_t value);
