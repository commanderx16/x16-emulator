// Commander X16 Emulator
// Copyright (c) 2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _RTC_H_
#define _RTC_H_

#include <stdint.h>

void rtc_init(bool set_system_time);
void rtc_set_system_time();
void rtc_step(int c);
uint8_t rtc_read(uint8_t offset);
void rtc_write(uint8_t offset, uint8_t value);

#endif
