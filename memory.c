#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "glue.h"
#include "memory.h"
#include "video.h"

uint16_t ram_bot, ram_top;

//
// interface for fake6502
//

uint8_t RAM[65536];

uint8_t
read6502(uint16_t address)
{
	if (address >= 0x9f00 && address < 0xa000) {
		// I/O
		if (address >= 0x9f00 && address < 0x9f20) {
			// TODO: sound
			return 0;
		} else if (address >= 0x9f20 && address < 0x9f28) {
			return video_read(address & 7);
		} else if (address >= 0x9f40 && address < 0x9f60) {
			// TODO: character LCD
			return 0;
		} else if (address >= 0x9f60 && address < 0x9f70) {
			return via1_read(address & 0xf);
		} else if (address >= 0x9f70 && address < 0x9f80) {
			// TODO: VIA #2
			return 0;
		} else if (address >= 0x9f80 && address < 0x9fa0) {
			// TODO: RTC
			return 0;
		} else {
			return 0;
		}
	} else {
		// RAM/ROM
		return RAM[address];
	}
}

void
write6502(uint16_t address, uint8_t value)
{
	if (address < 0x9f00) {
		RAM[address] = value;
	} else {
		if (address >= 0x9f00 && address < 0x9f20) {
			// TODO: sound
		} else if (address >= 0x9f20 && address < 0x9f40) {
			video_write(address & 7, value);
		} else if (address >= 0x9f40 && address < 0x9f60) {
			// TODO: character LCD
		} else if (address >= 0x9f60 && address < 0x9f70) {
			via1_write(address & 7, value);
		} else if (address >= 0x9f70 && address < 0x9f80) {
			// TODO: VIA #2
		} else if (address >= 0x9f80 && address < 0x9fa0) {
			// TODO: RTC
		} else {
			// future expansion
		}
	}
}

void
memory_save()
{
	int index = 0;
	char filename[20];
	for (;;) {
		if (!index) {
			strcpy(filename, "memory.bin");
		} else {
			sprintf(filename, "memory-%i.bin", index);
		}
		if (access(filename, F_OK) == -1) {
			break;
		}
		index++;
	}
	FILE *f = fopen(filename, "w");
	if (!f) {
		printf("Cannot write to %s!\n", filename);
		return;
	}
	fwrite(RAM, 65536, 1, f);
	fclose(f);
	printf("Saved memory contents to %s.\n", filename);
}
