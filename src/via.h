// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef VIA_H
#define VIA_H

#include <stdint.h>

void via1_init();
uint8_t via1_read(uint8_t reg);
void via1_write(uint8_t reg, uint8_t value);

void via2_init();
uint8_t via2_read(uint8_t reg);
void via2_write(uint8_t reg, uint8_t value);

#endif
