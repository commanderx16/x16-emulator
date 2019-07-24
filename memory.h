#include <stdint.h>

uint8_t read6502(uint16_t address);

void memory_save();

void memory_set_ram_bank(uint8_t bank);
void memory_set_rom_bank(uint8_t bank);

