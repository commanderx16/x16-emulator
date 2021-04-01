// Commander X16 Emulator
// Copyright (c) 2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "rtc.h"

uint8_t
rtc_read(uint8_t offset) {
    return 0xff;
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

