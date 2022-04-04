// Commander X16 Emulator
// Copyright (c) 2022 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "serial.h"
#include "ieee.h"
#include "glue.h"

serial_port_t serial_port;

static int state = 0;
static bool valid;
static int bit;
static uint8_t byte;
static bool listening = false;
static bool talking = false;
static bool during_atn = false;
static bool eoi = false;
static bool fnf = false; // file not found
static int clocks_since_last_change = 0;

//#define printf(...)

static uint8_t
read_byte(bool *eoi) {
	uint8_t byte;
	int ret = ACPTR(&byte);
	*eoi = ret >= 0;
	return byte;
}

void
serial_step(int clocks)
{
	bool print = false;

	static serial_port_t old_serial_port;
	if (old_serial_port.atn_in == serial_port.atn_in &&
		old_serial_port.clk_in == serial_port.clk_in &&
		old_serial_port.data_in == serial_port.data_in) {
		clocks_since_last_change += clocks;
		if (state == 2 && valid == true && bit == 0 && clocks_since_last_change > 200 * MHZ) {
			if (clocks_since_last_change < (200 + 60) * MHZ) {
				printf("XXX EOI ACK\n");
				serial_port.data_out = 0;
				eoi = true;
			} else {
				printf("XXX EOI ACK END\n");
				serial_port.data_out = 1;
				clocks_since_last_change = 0;
			}
			print = true;
		}
		if (state == 10 && clocks_since_last_change > 60 * MHZ) {
			serial_port.clk_out = 1;
			state = 11;
			clocks_since_last_change = 0;
			print = true;
		} else if (state == 11 && serial_port.data_in && !fnf) {
			clocks_since_last_change = 0;
			byte = read_byte(&eoi);
			bit = 0;
			valid = true;
			if (eoi) {
				printf("XXXEOI1\n");
				state = 12;
			} else {
				printf("XXXEOI0\n");
				serial_port.clk_out = 0;
				state = 13;
			}
			print = true;
		} else if (state == 12 && clocks_since_last_change > 512 * MHZ) {
			// EOI delay
			// XXX we'd have to check for the ACK
			clocks_since_last_change = 0;
			serial_port.clk_out = 0;
			state = 13;
			print = true;
		} else if (state == 13 && clocks_since_last_change > 60 * MHZ) {
			if (valid) {
				// send bit
				serial_port.data_out = (byte >> bit) & 1;
				serial_port.clk_out = 1;
				printf("*** BIT%d OUT: %d\n", bit, serial_port.data_out);
				bit++;
				if (bit == 8) {
					state = 14;
				}
			} else {
				serial_port.clk_out = 0;
			}
			valid = !valid;
			clocks_since_last_change = 0;
			print = true;
		} else if (state == 14 && clocks_since_last_change > 60 * MHZ) {
			serial_port.data_out = 1;
			serial_port.clk_out = 0;
			state = 10;
			clocks_since_last_change = 0;
			print = true;
		}
	} else {
		clocks_since_last_change = 0;

		printf("-SERIAL IN { ATN:%d CLK:%d DATA:%d } --- OUT { CLK:%d DATA:%d }\n", old_serial_port.atn_in, old_serial_port.clk_in, old_serial_port.data_in, old_serial_port.clk_out, old_serial_port.data_out);
		printf("+SERIAL IN { ATN:%d CLK:%d DATA:%d } --- OUT { CLK:%d DATA:%d } -- #%d\n", serial_port.atn_in, serial_port.clk_in, serial_port.data_in, serial_port.clk_out, serial_port.data_out, state);

		if (!during_atn && serial_port.atn_in) {
			serial_port.data_out = 0;
			state = 99;
			during_atn = true;
			printf("XXX START OF ATN\n");
		}

		switch(state) {
			case 0:
				break;
			case 99:
				// wait for CLK=0
				if (!serial_port.clk_in) {
					state = 1;
				}
				break;
			case 1:
				if (during_atn && !serial_port.atn_in) {
					// cancelled ATN
					serial_port.data_out = 1;
					serial_port.clk_out = 1;
					during_atn = false;
					printf("*** END OF ATN\n");
					if (listening) {
						// keep holding DATA to indicate we're here
						serial_port.data_out = 0;
						printf("XXX START OF DATA RECEIVE\n");
					} else if (talking) {
						serial_port.clk_out = 0;
						state = 10;
						printf("XXX START OF DATA SEND\n");
					} else {
						state = 0;
					}
					break;
				}
				// wait for CLK=1
				if (serial_port.clk_in) {
					serial_port.data_out = 1;
					state = 2;
					valid = true;
					bit = 0;
					byte = 0;
					eoi = false;
					printf("XXX START OF BYTE\n");
				}
				break;
			case 2:
				if (during_atn && !serial_port.atn_in) {
					// cancelled ATN
					serial_port.data_out = 1;
					serial_port.clk_out = 1;
					printf("*** XEND OF ATN\n");
					state = 0;
					break;
				}
				if (valid) {
					// wait for CLK=0, data not valid
					if (!serial_port.clk_in) {
						valid = false;
						printf("XXX NOT VALID\n");
					}
				} else {
					// wait for CLK=1, data valid
					if (serial_port.clk_in) {
						bool b = serial_port.data_in;
						byte |= (b << bit);
						printf("*** BIT%d IN: %d\n", bit, b);
						valid = true;
						if (++bit == 8) {
							printf("*** %s BYTE IN: %02x%s\n", during_atn ? "ATN" : "DATA", byte, eoi ? " (EOI)" : "");
							if (during_atn) {
								printf("IEEE CMD %x\n", byte);
								switch (byte & 0x60) {
									case 0x20:
										if (byte == 0x3f) {
											int ret = UNLSN(byte);
											fnf = ret == 2;
											listening = false;
										} else {
											LISTEN();
											listening = true;
										}
										break;
									case 0x40:
										if (byte == 0x5f) {
											UNTLK(byte);
											talking = false;
										} else {
											TALK(byte);
											talking = true;
										}
										break;
									case 0x60:
										if (listening) {
											SECOND(byte);
										} else { // talking
											TKSA(byte);
										}
										break;
								}
							} else {
								CIOUT(byte);
							}
							serial_port.data_out = 0;
							state = 1;
						}
					}
				}
				break;
			case 3:
				break;
		}
		print = true;
	}

	if (print) {
		printf(">SERIAL IN { ATN:%d CLK:%d DATA:%d } --- OUT { CLK:%d DATA:%d } -- #%d\n", serial_port.atn_in, serial_port.clk_in, serial_port.data_in, serial_port.clk_out, serial_port.data_out, state);
	}
	old_serial_port = serial_port;
}

