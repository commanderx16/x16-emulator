// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "sdcard.h"
#include "via.h"

// VIA#2
// PB0 SPICLK
// PB1 SS
// PB2-4 unassigned
// PB5 SD write protect
// PB6 SD detect
// PB7 MOSI
// CB1 SPICLK (=PB0)
// CB2 MISO

FILE *sdcard_file = NULL;

static bool initialized;

void
sdcard_init()
{
	initialized = false;
}

void
sdcard_step()
{
	if (!sdcard_file) {
		return;
	}

	uint8_t port = via2_pb_get_out();
	bool clk = port & 1;
	bool ss = !((port >> 1) & 1);
	bool mosi = port >> 7;

	static int cmd_receive_counter;
	static uint8_t cmd[6];

	static bool last_clk = false;
	static bool last_ss;
	static int bit_counter = 0;

	// only care about rising clock
	if (clk == last_clk) {
		return;
	}
	last_clk = clk;
	if (clk == 0) {
		return;
	}

	if (ss && !last_ss) {
		bit_counter = 0;
		cmd_receive_counter = 0;
	}
	last_ss = ss;

	// For initialization, the client has to pull&release CLK 74 times.
	// The SD card should be deselected, because it's not actual
	// data transmission (we ignore this).
	if (!initialized) {
		if (clk == 1) {
			static int init_counter = 0;
			init_counter++;
			if (init_counter >= 70) {
				cmd_receive_counter = 0;
				initialized = true;
			}
		}
		return;
	}

	// for everything else, the SD card neeeds to be selcted
	if (!ss) {
		return;
	}

	// receive byte
	static uint8_t inbyte, outbyte;
	bool bit = mosi;
	inbyte <<= 1;
	inbyte |= bit;
//	printf("BIT: %d BYTE =$%02x\n", bit, inbyte);
	bit_counter++;
	if (bit_counter != 8) {
		return;
	}

	bit_counter = 0;

	static const uint8_t *response = NULL;
	static int response_length = 0;
	static int response_counter = 0;

	if (initialized) {
		if (cmd_receive_counter == 0 && inbyte == 0xff) {
			// send response data
			if (response) {
				outbyte = response[response_counter++];
				if (response_counter == response_length) {
					response = NULL;
				}
			} else {
				outbyte = 0xff;
			}
		} else {
			cmd[cmd_receive_counter++] = inbyte;
			if (cmd_receive_counter == 6) {
				cmd_receive_counter = 0;

//				printf("*** COMMAND: $40 + %d\n", cmd[0] - 0x40);
				switch (cmd[0]) {
					case 0x40: {
						// CMD0: init SD card to SPI mode
						static const uint8_t r[] = { 1 };
						response = r;
						response_length = sizeof(r);
						break;
					}
					case 0x40 + 8: {
						static const uint8_t r[] = { 1, 0x00, 0x00, 0x01, 0xaa };
						response = r;
						response_length = sizeof(r);
						break;
					}
					case 0x40 + 41: {
						static const uint8_t r[] = { 0 };
						response = r;
						response_length = sizeof(r);
						break;
					}
					case 0x40 + 16: {
						// set block size
						printf("Set block size = 0x%04x\n", cmd[2] << 8 | cmd[3]);
						static const uint8_t r[] = { 1 };
						response = r;
						response_length = sizeof(r);
						break;
					}
					case 0x40 + 17: {
						// read block
						uint32_t lba =
							cmd[1] << 24 |
							cmd[2] << 16 |
							cmd[3] << 8 |
							cmd[4];
						static uint8_t read_block_respose[2 + 512 + 2];
						read_block_respose[0] = 0;
						read_block_respose[1] = 0xfe;
						printf("Reading LBA %d\n", lba);
						fseek(sdcard_file, lba * 512, SEEK_SET);
						int bytes_read = fread(&read_block_respose[2], 1, 512, sdcard_file);
						if (bytes_read != 512) {
							printf("Warning: short read!\n");
						}
						response = read_block_respose;
						response_length = 2 + 512 + 2;
						break;
					}
					case 0x40 + 55: {
						static const uint8_t r[] = { 1 };
						response = r;
						response_length = sizeof(r);
						break;
					}
					case 0x40 + 58: {
						static const uint8_t r[] = { 0, 0, 0, 0 };
						response = r;
						response_length = sizeof(r);
						break;
					}
					default: {
						static const uint8_t r[] = { 0 };
						response = r;
						response_length = sizeof(r);
						break;
					}
				}
				response_counter = 0;
			}
			outbyte = 0xff;
		}
	}
//	printf("$%02x ->$%02x\n", inbyte, outbyte);

	// send byte
	via2_sr_set(outbyte);
}
