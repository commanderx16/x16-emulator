// Commander X16 Emulator
// Copyright (c) 2021 Michael Steil
// All rights reserved. License: 2-clause BSD

// MCP7940N RTC

#include <stdio.h>
#include <stdbool.h>
#include "rtc.h"

#define BCD(a) ((a / 10) << 4 | (a % 10))

int datetime_seconds = 0;
int datetime_minutes = 35;
int datetime_hours = 16;
int datetime_day = 1;
int datetime_month = 4;
int datetime_year = 21;

uint8_t
rtc_read(uint8_t offset) {
    switch (offset) {
        case 0:
            return BCD(datetime_seconds);
        case 1:
            return BCD(datetime_minutes);
        case 2:
            return BCD(datetime_hours);
        case 4:
            return BCD(datetime_day);
        case 5:
            return BCD(datetime_month);
        case 6:
            return BCD(datetime_year);
    }
    return 0;
}

void
rtc_write(uint8_t offset, uint8_t value) {
    switch (offset) {
//        case 5:
//            led_status = value;
//        default:
            // no-op
    }
}

