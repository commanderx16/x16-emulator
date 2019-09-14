// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _VIA_H_
#define _VIA_H_

#include <stdint.h>

void via1_init();
uint8_t via1_read(uint8_t reg);
void via1_write(uint8_t reg, uint8_t value);
uint8_t via2_read(uint8_t reg);
void via2_write(uint8_t reg, uint8_t value);

void via2_init();
uint8_t via2_pb_get_out();
void via2_pb_set_in(uint8_t value);
void via2_sr_set(uint8_t value);

#endif
