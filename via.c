#include "via.h"
#include "ps2.h"

//
// VIA#1
//
// PA0-7 RAM BANK
// PB0-2 ROM BANK
// PB3   IECATT0
// PB4   IECCLK0
// PB5   IECDAT0
// PB6   IECCLK
// PB7   IECDAT
// CB1   IECSRQ

uint8_t
via1_read(uint8_t reg)
{
	// TODO
	return 0;
}

void
via1_write(uint8_t reg, uint8_t value)
{
	// TODO
}

//
// VIA#2
//
// PA0 PS/2 DAT
// PA1 PS/2 CLK
// PA2 LCD backlight
// PA3 NESJOY latch (for both joysticks)
// PA4 NESJOY joy1 data
// PA5 NESJOY joy1 CLK
// PA6 NESJOY joy2 data
// PA7 NESJOY joy2 CLK

uint8_t
via2_read(uint8_t reg)
{
	if (reg == 0) { // PORT B: fake scan code for now
		uint8_t code = kbd_buffer_remove();
		if (code) {
			//			printf("VIA1PB: $%02x\n", code);
		}
		return code;
	} else {
		return 0;
	}
}

void
via2_write(uint8_t reg, uint8_t value)
{
	// TODO
}

