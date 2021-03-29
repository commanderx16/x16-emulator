// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <inttypes.h>

void vera_spi_init();
void vera_spi_step();
uint8_t vera_spi_read(uint8_t address);
void vera_spi_write(uint8_t address, uint8_t value);
