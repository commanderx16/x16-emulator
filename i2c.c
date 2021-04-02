// Commander X16 Emulator
// Copyright (c) 2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "i2c.h"
#include "smc.h"
#include "rtc.h"

#define LOG_LEVEL 0

#define DEVICE_SMC 0x42
#define DEVICE_RTC 0x6F

#define STATE_START 0
#define STATE_STOP -1

i2c_port_t i2c_port;

static int state = STATE_STOP;
static bool read_mode = false;
static uint8_t byte = 0;
static int count = 0;
static uint8_t device;
static uint8_t offset;

uint8_t
i2c_read(uint8_t device, uint8_t offset) {
	uint8_t value;
	switch (device) {
		case DEVICE_SMC:
			return smc_read(offset);
		case DEVICE_RTC:
			return rtc_read(offset);
		default:
			value = 0xff;
	}
#if LOG_LEVEL >= 1
	printf("I2C READ($%02X:$%02X) = $%02X\n", device, offset, value);
#endif
	return value;
}

void
i2c_write(uint8_t device, uint8_t offset, uint8_t value) {
	switch (device) {
		case DEVICE_SMC:
			smc_write(offset, value);
			break;
		case DEVICE_RTC:
			rtc_write(offset, value);
			break;
			//        default:
			// no-op
	}
#if LOG_LEVEL >= 1
	printf("I2C WRITE $%02X:$%02X, $%02X\n", device, offset, byte);
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
			i2c_port.data_out = 1;
			if (state < 8) {
				if (read_mode) {
					if (state == 0) {
						byte = i2c_read(device, offset);
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
					bool nack = i2c_port.data_in;
					if (nack) {
#if LOG_LEVEL >= 3
					printf("I2C OUT DONE (NACK)\n");
#endif
						count = 0;
						read_mode = false;
					} else {
#if LOG_LEVEL >= 3
					printf("I2C OUT DONE (ACK)\n");
#endif
						offset++;
					}
				} else {
					bool ack = true;
					switch (count) {
						case 0:
							device = byte >> 1;
							read_mode = byte & 1;
							if (device != DEVICE_SMC && device != DEVICE_RTC) {
								ack = false;
							}
							break;
						case 1:
							offset = byte;
							break;
						default:
							i2c_write(device, offset, byte);
							offset++;
							break;
					}
					if (ack) {
#if LOG_LEVEL >= 3
						printf("I2C ACK(%d) $%02X\n", count, byte);
#endif
						i2c_port.data_out = 0;
						count++;
					} else {
#if LOG_LEVEL >= 3
						printf("I2C NACK(%d) $%02X\n", count, byte);
#endif
						count = 0;
						read_mode = false;
					}
				}
				state = STATE_START;
			}
		}
		old_i2c_port = i2c_port;
	}
}
