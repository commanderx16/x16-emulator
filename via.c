#include "via.h"
#include "ps2.h"

//
// VIA#1
//

uint8_t
via1_read(uint8_t reg)
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
via1_write(uint8_t reg, uint8_t value)
{
	// TODO
}
