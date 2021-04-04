// Commander X16 Emulator
// Copyright (c) 2021 Michael Steil
// All rights reserved. License: 2-clause BSD

// System Management Controller

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "smc.h"
#include "glue.h"

// 0x01 0x00      - Power Off
// 0x01 0x01      - Hard Reboot
// 0x02 0x00      - Reset Button Press
// 0x03 0x00      - NMI Button Press
// 0x04 0x00-0xFF - Power LED Level (PWM)
// 0x05 0x00-0xFF - Activity LED Level (PWM)

uint8_t activity_led;

uint8_t
smc_read(uint8_t a) {
	return 0xff;
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

