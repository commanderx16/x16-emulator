// Commander X16 Emulator
// Copyright (c) 2020 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _RTC_H_
#define _RTC_H_

#include <stdint.h>

void rtc_init();
void rtc_step(int clocks);
uint8_t rtc_read(uint8_t reg);
void rtc_write(uint8_t reg, uint8_t value);

#endif
