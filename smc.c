// Commander X16 Emulator
// Copyright (c) 2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "smc.h"

uint8_t led_status;

uint8_t
smc_read(uint8_t offset) {
    return 0xff;
}

void
smc_write(uint8_t offset, uint8_t value) {
    switch (offset) {
        case 5:
            led_status = value;
//        default:
            // no-op
    }
}

