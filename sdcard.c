// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdbool.h>
#include "sdcard.h"

static int cmd_receive_counter;
static int data_receive_counter;
static int data_receive_mode = false;
static int data_receive_triggered = false;
static uint32_t write_lba;
static uint8_t write_data[512];
SDL_RWops *sdcard_file = NULL;

void
sdcard_select()
{
	cmd_receive_counter = 0;
}

uint8_t
sdcard_handle(uint8_t inbyte)
{
	static uint8_t cmd[6];

	static const uint8_t *response = NULL;
	static int response_length = 0;
	static int response_counter = 0;
	uint8_t outbyte;

	if (!sdcard_file) {
		// no SD card connected
		return 0xff;
	}

	 if (response && cmd_receive_counter == 0 && inbyte == 0xff) {
		// send response data
		outbyte = response[response_counter++];
		if (response_counter == response_length) {
			response = NULL;
		}
	} else if (data_receive_mode) {
		if (data_receive_triggered) {
			write_data[data_receive_counter] = inbyte;
			data_receive_counter++;
			if (data_receive_counter == 512) {
				SDL_RWseek(sdcard_file, write_lba * 512, RW_SEEK_SET);
				size_t bytes_written = SDL_RWwrite(sdcard_file, write_data, 1, 512);
				if (bytes_written != 512) {
					printf("Warning: short write!\n");
				}

				data_receive_mode = false;
			}
		} else if (inbyte == 0xfe) {
			data_receive_triggered = true;
		}
		outbyte = 0xff;
	} else if (cmd_receive_counter == 0 && inbyte == 0xff) {
		outbyte = 0xff;
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
//					printf("Set block size = 0x%04x\n", cmd[2] << 8 | cmd[3]);
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
//					printf("Reading LBA %d\n", lba);
					SDL_RWseek(sdcard_file, lba * 512, SEEK_SET);
					size_t bytes_read = SDL_RWread(sdcard_file, &read_block_respose[2], 1, 512);
					if (bytes_read != 512) {
						printf("Warning: short read!\n");
					}
					response = read_block_respose;
					response_length = 2 + 512 + 2;
					break;
				}
				case 0x40 + 24: {
					// write block
					write_lba =
						cmd[1] << 24 |
						cmd[2] << 16 |
						cmd[3] << 8 |
						cmd[4];

					data_receive_mode = true;
					data_receive_triggered = false;
					data_receive_counter = 0;
					static const uint8_t r[] = { 0 };
					response = r;
					response_length = sizeof(r);
					break;
				}
				case 0x40 + 55: {
					static const uint8_t r[] = { 1 };
					response = r;
					response_length = sizeof(r);
					break;
				}
				case 0x40 + 58: {
					static const uint8_t r[] = { 0, 0x40, 0, 0 };
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
//	printf("$%02x ->$%02x\n", inbyte, outbyte);

	return outbyte;
}

