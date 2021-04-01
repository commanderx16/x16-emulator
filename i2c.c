// Commander X16 Emulator
// Copyright (c) 2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "i2c.h"

#define LOG_LEVEL 1

i2c_port_t i2c_port;

#define STATE_START 0
#define STATE_STOP -1

int state = STATE_STOP;
bool read_mode = false;
uint8_t byte = 0;
int count = 0;
uint8_t device;
uint8_t offset;

uint8_t
i2c_peek(uint8_t device, uint8_t offset) {
    uint8_t value = 0xea;
#if LOG_LEVEL >= 1
    printf("I2C PEEK($%02X:$%02X) = $%02X\n", device, offset, value);
#endif
    return value;
}

void
i2c_poke(uint8_t device, uint8_t offset, uint8_t value) {
#if LOG_LEVEL >= 1
    printf("I2C POKE $%02X:$%02X, $%02X\n", device, offset, byte);
#endif
}

void
i2c_step()
{
    static i2c_port_t old_i2c_port;
    if (old_i2c_port.clk_in != i2c_port.clk_in || old_i2c_port.data_in != i2c_port.data_in) {
#if LOG_LEVEL >= 5
        printf("I2C(%d) C:%d D:%d\n", state, i2c_port.clk_in, i2c_port.data_in);
#endif
        if (state == STATE_STOP && i2c_port.clk_in == 0 && i2c_port.data_in == 0) {
#if LOG_LEVEL >= 2
            printf("I2C START\n");
#endif
            state = STATE_START;
        }
        if (state == 1 && i2c_port.clk_in == 1 &&i2c_port.data_in == 1 &&  old_i2c_port.data_in == 0) {
#if LOG_LEVEL >= 2
            printf("I2C STOP\n");
#endif
            state = STATE_STOP;
            count = 0;
            read_mode = false;
        }
        if (state != STATE_STOP && i2c_port.clk_in == 1 && old_i2c_port.clk_in == 0) {
            i2c_port.clk_out = 1;
            i2c_port.data_out = 1;
            if (state < 8) {
                if (read_mode) {
                    if (state == 0) {
                        byte = i2c_peek(device, offset);
                    }
                    i2c_port.data_out = !!(byte & 0x80);
                    byte <<= 1;
#if LOG_LEVEL >= 4
                    printf("I2C OUT#%d: %d\n", state, i2c_port.data_out);
#endif
                    state++;
                } else {
#if LOG_LEVEL >= 4
                    printf("I2C BIT#%d: %d\n", state, i2c_port.data_in);
#endif
                    byte <<= 1;
                    byte |= i2c_port.data_in;
                    state++;
                }
            } else { // state == 8
                if (read_mode) {
#if LOG_LEVEL >= 3
                    printf("I2C OUT DONE\n");
#endif
                    __unused bool ack = i2c_port.data_in;
                    offset++;
                } else {
#if LOG_LEVEL >= 3
                    printf("I2C ACK(%d) $%02X\n", count, byte);
#endif
                    i2c_port.data_out = 0;
                    switch (count) {
                        case 0:
                            device = byte >> 1;
                            read_mode = byte & 1;
                            break;
                        case 1:
                            offset = byte;
                            break;
                        default:
                            i2c_poke(device, offset, byte);
                            offset++;
                            break;
                    }
                    count++;
                }
                state = STATE_START;
            }
        }
        old_i2c_port = i2c_port;
    }
}
