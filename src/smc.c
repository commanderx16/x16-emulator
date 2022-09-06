// Commander X16 Emulator
// Copyright (c) 2021 Michael Steil
// All rights reserved. License: 2-clause BSD

// System Management Controller

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "smc.h"
#include "glue.h"
#include "i2c.h"

// 0x01 0x00      - Power Off
// 0x01 0x01      - Hard Reboot
// 0x02 0x00      - Reset Button Press
// 0x03 0x00      - NMI Button Press
// 0x04 0x00-0xFF - Power LED Level (PWM)
// 0x05 0x00-0xFF - Activity LED Level (PWM)

uint8_t activity_led;
uint8_t mse_count = 0;

uint8_t
smc_read(uint8_t a) {
	switch (a){
		//Offset that returns one byte from the keyboard buffer
		case 7:
			return i2c_kbd_buffer_next();

		//Offset that returns three bytes from mouse buffer (one movement packet) or a single zero if there is not complete packet in the buffer
		//mse_count keeps track of which one of the three bytes it's sending
		case 0x21:
			if (mse_count==0 && i2c_mse_buffer_count()>2){			//If start of packet, check if there are at least three bytes in the buffer
				mse_count++;
				return i2c_mse_buffer_next();
			}
			else if (mse_count>0){									//If we have already started sending bytes, assume there is enough data in the buffer
				mse_count++;
				if (mse_count==3) mse_count = 0;
				return i2c_mse_buffer_next();
			}
			else{													//Return a single zero if no complete packet available
				mse_count=0;
				return 0x00;
			}

		default:
			return 0xff;
	}
}

void
smc_write(uint8_t a, uint8_t v) {
	switch (a) {
		case 1:
			if (v == 0) {
				printf("SMC Power Off.\n");
				exit(0);
			} else if (v == 1) {
				machine_reset();
			}
			break;
		case 2:
			if (v == 0) {
				machine_reset();
			}
			break;
		case 3:
			if (v == 0) {
				// TODO NMI
			}
			break;
		case 4:
			// TODO power LED
			break;
		case 5:
			activity_led = v;
			break;
	}
}

