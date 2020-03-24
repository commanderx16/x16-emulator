// Commander X16 Emulator
// Copyright (c) 2020 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _IO_H_
#define _IO_H_

#include <stdint.h>
#include <stdbool.h>

void io_init(void);

uint8_t via1_get_pa(void);
void via1_set_pa(uint8_t pinstate);
uint8_t via1_get_pb(void);
void via1_set_pb(uint8_t pinstate);
bool via1_get_ca1(void);
bool via1_get_cb1(void);

uint8_t via2_get_pa(void);
void via2_set_pa(uint8_t pinstate);
uint8_t via2_get_pb(void);
void via2_set_pb(uint8_t pinstate);
bool via2_get_ca1(void);
bool via2_get_cb1(void);

#endif
