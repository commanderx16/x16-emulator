#include <stdio.h>
#include <stdbool.h>
#include "glue.h"
#include "ps2.h"
#include "keyboard.h"

#define EXTENDED_FLAG 0x100
#define ESC_IS_BREAK /* if enabled, Esc sends Break/Pause key instead of Esc */

int
ps2_scancode_from_SDL_Scancode(SDL_Scancode scancode)
{
	switch (scancode) {
		case SDL_SCANCODE_GRAVE:
			return 0x0e;
		case SDL_SCANCODE_BACKSPACE:
			return 0x66;
		case SDL_SCANCODE_TAB:
			return 0xd;
		case SDL_SCANCODE_CLEAR:
			return 0;
		case SDL_SCANCODE_RETURN:
			return 0x5a;
		case SDL_SCANCODE_PAUSE:
			return 0;
		case SDL_SCANCODE_ESCAPE:
#ifdef ESC_IS_BREAK
			return 0xff;
#else
			return 0x76;
#endif
		case SDL_SCANCODE_SPACE:
			return 0x29;
		case SDL_SCANCODE_APOSTROPHE:
			return 0x52;
		case SDL_SCANCODE_COMMA:
			return 0x41;
		case SDL_SCANCODE_MINUS:
			return 0x4e;
		case SDL_SCANCODE_PERIOD:
			return 0x49;
		case SDL_SCANCODE_SLASH:
			return 0x4a;
		case SDL_SCANCODE_0:
			return 0x45;
		case SDL_SCANCODE_1:
			return 0x16;
		case SDL_SCANCODE_2:
			return 0x1e;
		case SDL_SCANCODE_3:
			return 0x26;
		case SDL_SCANCODE_4:
			return 0x25;
		case SDL_SCANCODE_5:
			return 0x2e;
		case SDL_SCANCODE_6:
			return 0x36;
		case SDL_SCANCODE_7:
			return 0x3d;
		case SDL_SCANCODE_8:
			return 0x3e;
		case SDL_SCANCODE_9:
			return 0x46;
		case SDL_SCANCODE_SEMICOLON:
			return 0x4c;
		case SDL_SCANCODE_EQUALS:
			return 0x55;
		case SDL_SCANCODE_LEFTBRACKET:
			return 0x54;
		case SDL_SCANCODE_BACKSLASH:
			return 0x5d;
		case SDL_SCANCODE_RIGHTBRACKET:
			return 0x5b;
		case SDL_SCANCODE_A:
			return 0x1c;
		case SDL_SCANCODE_B:
			return 0x32;
		case SDL_SCANCODE_C:
			return 0x21;
		case SDL_SCANCODE_D:
			return 0x23;
		case SDL_SCANCODE_E:
			return 0x24;
		case SDL_SCANCODE_F:
			return 0x2b;
		case SDL_SCANCODE_G:
			return 0x34;
		case SDL_SCANCODE_H:
			return 0x33;
		case SDL_SCANCODE_I:
			return 0x43;
		case SDL_SCANCODE_J:
			return 0x3B;
		case SDL_SCANCODE_K:
			return 0x42;
		case SDL_SCANCODE_L:
			return 0x4B;
		case SDL_SCANCODE_M:
			return 0x3A;
		case SDL_SCANCODE_N:
			return 0x31;
		case SDL_SCANCODE_O:
			return 0x44;
		case SDL_SCANCODE_P:
			return 0x4D;
		case SDL_SCANCODE_Q:
			return 0x15;
		case SDL_SCANCODE_R:
			return 0x2D;
		case SDL_SCANCODE_S:
			return 0x1B;
		case SDL_SCANCODE_T:
			return 0x2C;
		case SDL_SCANCODE_U:
			return 0x3C;
		case SDL_SCANCODE_V:
			return 0x2A;
		case SDL_SCANCODE_W:
			return 0x1D;
		case SDL_SCANCODE_X:
			return 0x22;
		case SDL_SCANCODE_Y:
			return 0x35;
		case SDL_SCANCODE_Z:
			return 0x1A;
		case SDL_SCANCODE_DELETE:
			return 0;
		case SDL_SCANCODE_UP:
			return 0x75 | EXTENDED_FLAG;
		case SDL_SCANCODE_DOWN:
			return 0x72 | EXTENDED_FLAG;
		case SDL_SCANCODE_RIGHT:
			return 0x74 | EXTENDED_FLAG;
		case SDL_SCANCODE_LEFT:
			return 0x6b | EXTENDED_FLAG;
		case SDL_SCANCODE_INSERT:
			return 0;
		case SDL_SCANCODE_HOME:
			return 0x6c | EXTENDED_FLAG;
		case SDL_SCANCODE_END:
			return 0;
		case SDL_SCANCODE_PAGEUP:
			return 0;
		case SDL_SCANCODE_PAGEDOWN:
			return 0;
		case SDL_SCANCODE_F1:
			return 0x05;
		case SDL_SCANCODE_F2:
			return 0x06;
		case SDL_SCANCODE_F3:
			return 0x04;
		case SDL_SCANCODE_F4:
			return 0x0c;
		case SDL_SCANCODE_F5:
			return 0x03;
		case SDL_SCANCODE_F6:
			return 0x0b;
		case SDL_SCANCODE_F7:
			return 0x83;
		case SDL_SCANCODE_F8:
			return 0x0a;
		case SDL_SCANCODE_F9:
			return 0x01;
		case SDL_SCANCODE_F10:
			return 0x09;
		case SDL_SCANCODE_F11:
			return 0x78;
		case SDL_SCANCODE_F12:
			return 0x07;
		case SDL_SCANCODE_RSHIFT:
			return 0x59;
		case SDL_SCANCODE_LSHIFT:
			return 0x12;
		case SDL_SCANCODE_LCTRL:
			return 0x14;
		case SDL_SCANCODE_RCTRL:
			return 0x14 | EXTENDED_FLAG;
		case SDL_SCANCODE_LALT:
			return 0x11;
		case SDL_SCANCODE_RALT:
			return 0x11 | EXTENDED_FLAG;
//		case SDL_SCANCODE_LGUI: // Windows/Command
//			return 0x5b | EXTENDED_FLAG;
		case SDL_SCANCODE_NONUSBACKSLASH:
			return 0x61;
		case SDL_SCANCODE_KP_ENTER:
			return 0x5a | EXTENDED_FLAG;
		case SDL_SCANCODE_KP_0:
			return 0x70;
		case SDL_SCANCODE_KP_1:
			return 0x69;
		case SDL_SCANCODE_KP_2:
			return 0x72;
		case SDL_SCANCODE_KP_3:
			return 0x7a;
		case SDL_SCANCODE_KP_4:
			return 0x6b;
		case SDL_SCANCODE_KP_5:
			return 0x73;
		case SDL_SCANCODE_KP_6:
			return 0x74;
		case SDL_SCANCODE_KP_7:
			return 0x6c;
		case SDL_SCANCODE_KP_8:
			return 0x75;
		case SDL_SCANCODE_KP_9:
			return 0x7d;
		case SDL_SCANCODE_KP_PERIOD:
			return 0x71;
		case SDL_SCANCODE_KP_PLUS:
			return 0x79;
		case SDL_SCANCODE_KP_MINUS:
			return 0x7b;
		case SDL_SCANCODE_KP_MULTIPLY:
			return 0x7c;
		case SDL_SCANCODE_KP_DIVIDE:
			return 0x4a | EXTENDED_FLAG;
		default:
			return 0;
	}
}

void
handle_keyboard(bool down, SDL_Keycode sym, SDL_Scancode scancode)
{
	if (down) {
#if __APPLE__ && (TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE)
		// iOS device
		if (scancode == SDL_SCANCODE_MENU) {
			sendNotification("ShowKeyboard");
		}
#endif

		if (log_keyboard) {
				printf("DOWN 0x%02X\n", scancode);
				fflush(stdout);
			}

			int ps2_scancode = ps2_scancode_from_SDL_Scancode(scancode);
			if (ps2_scancode == 0xff) {
				// "Pause/Break" sequence
				ps2_buffer_add(0, 0xe1);
				ps2_buffer_add(0, 0x14);
				ps2_buffer_add(0, 0x77);
				ps2_buffer_add(0, 0xe1);
				ps2_buffer_add(0, 0xf0);
				ps2_buffer_add(0, 0x14);
				ps2_buffer_add(0, 0xf0);
				ps2_buffer_add(0, 0x77);
			} else {
				if (ps2_scancode & EXTENDED_FLAG) {
					ps2_buffer_add(0, 0xe0);
				}
				ps2_buffer_add(0, ps2_scancode & 0xff);
			}
		} else {
			if (log_keyboard) {
				printf("UP	 0x%02X\n", scancode);
				fflush(stdout);
			}

			int ps2_scancode = ps2_scancode_from_SDL_Scancode(scancode);
			if (ps2_scancode & EXTENDED_FLAG) {
				ps2_buffer_add(0, 0xe0);
			}
			ps2_buffer_add(0, 0xf0); // BREAK
			ps2_buffer_add(0, ps2_scancode & 0xff);
		}
}
