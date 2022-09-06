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
static uint8_t value = 0;
static int count = 0;
static uint8_t device;
static uint8_t offset;

#define KBD_SIZE 16
uint8_t kbd_buffer[KBD_SIZE];						//Ring buffer for key codes

#define MSE_SIZE 8
uint8_t mse_buffer[MSE_SIZE];						//Ring buffer for mouse movement data

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
	printf("I2C WRITE $%02X:$%02X, $%02X\n", device, offset, value);
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
						value = i2c_read(device, offset);
					}
					i2c_port.data_out = !!(value & 0x80);
					value <<= 1;
#if LOG_LEVEL >= 4
					printf("I2C OUT#%d: %d\n", state, i2c_port.data_out);
#endif
					state++;
				} else {
#if LOG_LEVEL >= 4
					printf("I2C BIT#%d: %d\n", state, i2c_port.data_in);
#endif
					value <<= 1;
					value |= i2c_port.data_in;
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
						if (!read_mode) offset++;							//Set I2C write bit by increasing offset by one; don't do that if we're in read_mode
					}
				} else {
					bool ack = true;
					switch (count) {
						case 0:
							device = value >> 1;
							read_mode = value & 1;
							if (device != DEVICE_SMC && device != DEVICE_RTC) {
								ack = false;
							}
							break;
						case 1:
							offset = value;
							break;
						default:
							i2c_write(device, offset, value);
							offset++;
							break;
					}
					if (ack) {
#if LOG_LEVEL >= 3
						printf("I2C ACK(%d) $%02X\n", count, value);
#endif
						i2c_port.data_out = 0;
						count++;
					} else {
#if LOG_LEVEL >= 3
						printf("I2C NACK(%d) $%02X\n", count, value);
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


/**
 * Keyboard buffer functions
 **/
uint8_t kbd_head=0;
uint8_t kbd_tail=0;

/**
 * Adds value to the keyboard ring buffer; the value is discarded if the buffer is full
 **/
void i2c_kbd_buffer_add(uint8_t value){
	uint8_t next = (kbd_head + 1) & (KBD_SIZE - 1);		//Next available index
    if (next != kbd_tail){								//next = tail => buffer full
    	kbd_buffer[kbd_head] = value;					//Set new value
        kbd_head = next;								//Update buffer head pointer
    }
}

/**
 * Returns the tail value from the keyboard ring buffer; if the buffer is empty, it returns 0
 **/
uint8_t i2c_kbd_buffer_next(){
	uint8_t value = 0;									//Prepare to return 0 of buffer is empty
	if (kbd_head!=kbd_tail){							//head = tail => empty
    	value = kbd_buffer[kbd_tail];					//Get value
        kbd_tail = (kbd_tail + 1) & (KBD_SIZE - 1);		//Update buffer tail pointer
    }
	return value;
}

/**
 * Clears the keyboard ring buffer
 **/
void i2c_kbd_buffer_flush(){
	kbd_tail = kbd_head = 0;
}

/**
 * Mouse buffer functions
 **/
uint8_t mse_head=0;
uint8_t mse_tail=0;

/**
 * Adds value to the mouse ring buffer; discards the value if the buffer is full
 **/
void i2c_mse_buffer_add(uint8_t value){
	uint8_t next = (mse_head + 1) & (MSE_SIZE - 1);				//Next available index
    if (next != mse_tail){										//next = tail => buffer full
    	mse_buffer[mse_head] = value;							//Set value
        mse_head = next;										//Update head pointer
    }
}

/**
 * Returns the tail value from the mouse ring buffer; if the buffer is empty, it returns 0
 **/
uint8_t i2c_mse_buffer_next(){
	uint8_t value = 0;											//Prepare to return 0 if buffer empty
	if (mse_head!=mse_tail){									//head = tail => buffer empty
    	value = mse_buffer[mse_tail];							//Get tail value
        mse_tail = (mse_tail + 1) & (MSE_SIZE - 1);				//Update tail pointer
    }
	return value;
}

/**
 * Clears the mouse ring buffer
 **/
void i2c_mse_buffer_flush(){
	mse_tail = mse_head = 0;
}

/**
 * Returns number of values currently stored in the ring buffer
 **/
uint8_t i2c_mse_buffer_count(){
    return (MSE_SIZE+mse_head-mse_tail)&(MSE_SIZE-1);
}


/**
 *  fake mouse
 **/

static uint8_t buttons;
static int16_t mouse_diff_x = 0;
static int16_t mouse_diff_y = 0;

// byte 0, bit 7: Y overflow
// byte 0, bit 6: X overflow
// byte 0, bit 5: Y sign bit
// byte 0, bit 4: X sign bit
// byte 0, bit 3: Always 1
// byte 0, bit 2: Middle Btn
// byte 0, bit 1: Right Btn
// byte 0, bit 0: Left Btn
// byte 2:        X Movement
// byte 3:        Y Movement

static bool
mouse_send(int x, int y, int b)
{
	if (i2c_mse_buffer_count()<5) {
		uint8_t byte0 =
		    ((y >> 9) & 1) << 5 |
		    ((x >> 9) & 1) << 4 |
		    1 << 3 |
		    b;
		i2c_mse_buffer_add(byte0);
		i2c_mse_buffer_add(x);
		i2c_mse_buffer_add(y);

		return true;
	} else {
		printf("buffer full, skipping...\n");
		return false;
	}
}

void
mouse_send_state(void)
{
	do {
		int send_diff_x = mouse_diff_x > 255 ? 255 : (mouse_diff_x < -256 ? -256 : mouse_diff_x);
		int send_diff_y = mouse_diff_y > 255 ? 255 : (mouse_diff_y < -256 ? -256 : mouse_diff_y);

		mouse_send(send_diff_x, send_diff_y, buttons);

		mouse_diff_x -= send_diff_x;
		mouse_diff_y -= send_diff_y;
	} while (mouse_diff_x != 0 && mouse_diff_y != 0);
}

void
mouse_button_down(int num)
{
	buttons |= 1 << num;
}

void
mouse_button_up(int num)
{
	buttons &= (1 << num) ^ 0xff;
}

void
mouse_move(int x, int y)
{
	mouse_diff_x += x;
	mouse_diff_y -= y;
}

uint8_t
mouse_read(uint8_t reg)
{
	return 0xff;
}