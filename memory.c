#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "glue.h"
#include "via.h"
#include "memory.h"
#include "video.h"

uint8_t ram_bank = 0;
uint8_t rom_bank = 0;

uint8_t RAM[RAM_SIZE];
uint8_t ROM[ROM_SIZE];

// ROM file layout:
//   0000-1FFF: bank 0       ($C000)
//   2000-3FFF: fixed KERNAL ($E000)
//   4000-5FFF: bank 1       ($C000)
//   6000-7FFF: bank 2       ($C000)
//   ...

//
// interface for fake6502
//

uint8_t
read6502(uint16_t address)
{
	if (address < 0x9f00) { // RAM
		return RAM[address];
	} else if (address < 0xa000) { // I/O
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
			return via2_read(address & 0xf);
		} else if (address >= 0x9f80 && address < 0x9fa0) {
			// TODO: RTC
			return 0;
		} else {
			return 0;
		}
	} else if (address < 0xc000) { // banked RAM
		return RAM[0xa000 + (ram_bank << 13) + address - 0xa000];
	} else if (address < 0xe000) { // banked ROM
		if (rom_bank == 0) {
			// BASIC is at offset 0 * 8192 in ROM
			return ROM[address - 0xc000];
		} else {
			// other banks are at offset (n + 1) * 8192 in ROM
			return ROM[((rom_bank + 1) << 13) + address - 0xc000];
		}
	} else { // fixed ROM
		// KERNAL is at offset 1 * 8192 in ROM
		return ROM[address - 0xe000 + 0x2000];
	}
}

void
write6502(uint16_t address, uint8_t value)
{
	if (address < 0x9f00) { // RAM
		RAM[address] = value;
	} else if (address < 0xa000) { // I/O
		if (address >= 0x9f00 && address < 0x9f20) {
			// TODO: sound
		} else if (address >= 0x9f20 && address < 0x9f40) {
			video_write(address & 7, value);
		} else if (address >= 0x9f40 && address < 0x9f60) {
			// TODO: character LCD
		} else if (address >= 0x9f60 && address < 0x9f70) {
			via1_write(address & 0xf, value);
		} else if (address >= 0x9f70 && address < 0x9f80) {
			via2_write(address & 0xf, value);
		} else if (address >= 0x9f80 && address < 0x9fa0) {
			// TODO: RTC
		} else {
			// future expansion
		}
	} else if (address < 0xc000) { // banked RAM
		RAM[0xa000 + (ram_bank << 13) + address - 0xa000] = value;
	} else { // ROM
		// ignore
	}
}

//
//
//

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
	fwrite(RAM, RAM_SIZE, 1, f);
	fclose(f);
	printf("Saved memory contents to %s.\n", filename);
}

//
//
//

void
memory_set_ram_bank(uint8_t bank)
{
	ram_bank = bank & (NUM_RAM_BANKS - 1);
}

void
memory_set_rom_bank(uint8_t bank)
{
	rom_bank = bank & (NUM_ROM_BANKS - 1);;
}
