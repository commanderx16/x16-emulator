#include "ps2.h"

#define KBD_BUFFER_SIZE 32
uint8_t kbd_buffer[KBD_BUFFER_SIZE];
uint8_t kbd_buffer_read = 0;
uint8_t kbd_buffer_write = 0;

void
kbd_buffer_add(uint8_t code)
{
	if ((kbd_buffer_write + 1) % KBD_BUFFER_SIZE == kbd_buffer_read) {
		// buffer full
		return;
	}

	kbd_buffer[kbd_buffer_write] = code;
	kbd_buffer_write = (kbd_buffer_write + 1) % KBD_BUFFER_SIZE;
}

uint8_t
kbd_buffer_remove()
{
	if (kbd_buffer_read == kbd_buffer_write) {
		return 0; // empty
	} else {
		uint8_t code = kbd_buffer[kbd_buffer_read];
		kbd_buffer_read = (kbd_buffer_read + 1) % KBD_BUFFER_SIZE;
		return code;
	}
}


