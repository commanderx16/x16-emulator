// Commander X16 Emulator
// Copyright (c) 2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "i2c.h"

i2c_port_t i2c_port;

#define STATE_START 0
#define STATE_STOP -1

int state = STATE_STOP;
uint8_t byte = 0;

void
i2c_step()
{
    static i2c_port_t old_i2c_port;
    if (old_i2c_port.clk_in != i2c_port.clk_in || old_i2c_port.data_in != i2c_port.data_in) {
        printf("I2C(%d) C:%d D:%d\n", state, i2c_port.clk_in, i2c_port.data_in);
        i2c_port.clk_out = 1;
        i2c_port.data_out = 1;
        if (state == STATE_STOP && i2c_port.clk_in == 0 && i2c_port.data_in == 0) {
            printf("I2C START\n");
            state = STATE_START;
        }
        if (state == 1 && i2c_port.data_in == 1 && old_i2c_port.data_in == 0) {
            printf("I2C STOP\n");
            state = STATE_STOP;
        }
        if (state >= 0 && i2c_port.clk_in == 1 && old_i2c_port.clk_in == 0) {
            if (state < 8) {
                printf("I2C BIT#%d: %d\n", state, i2c_port.data_in);
                byte <<= 1;
                byte |= i2c_port.data_in;
                state++;
            } else {
                printf("I2C ACK... $%02X\n", byte);
                i2c_port.data_out = 0;
                state = STATE_START;
            }
        }
        old_i2c_port = i2c_port;
    }
}
