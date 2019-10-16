/**********************************************/
// File     :     joystick.c
// Author   :     John Bliss
// Date     :     September 27th 2019
/**********************************************/

#include "joystick.h"


enum joy_status joy1_mode = NONE;
enum joy_status joy2_mode = NONE;


static SDL_GameController *joystick1 = NULL;
static SDL_GameController *joystick2 = NULL;
static bool old_clock = false;
static bool writing = false;
static uint16_t joystick1_state = 0;
static uint16_t joystick2_state = 0;
static uint8_t clock_count = 0;

bool joystick_latch, joystick_clock;
bool joystick1_data, joystick2_data;

bool joystick_init()
{
	int joystick1_number = -1;
	//Try to get first controller, if it is not set to 1
	if (joy1_mode != NONE) {
		for (int i = 0; i < SDL_NumJoysticks(); i++) {
			if (SDL_IsGameController(i)) {
				joystick1 = SDL_GameControllerOpen(i);
				if (joystick1) {
					joystick1_number = i;
					break;
				} else {
					fprintf(stderr, "Could not open gamecontroller %i: %s\n", i, SDL_GetError());
				}
			}
		}
	}
	if (joy2_mode != NONE) {
		for (int i=0; i < SDL_NumJoysticks(); i++) {
			if (SDL_IsGameController(i) && joystick1_number != i) {
				joystick2 = SDL_GameControllerOpen(i);
				if (joystick2) {
					break;
				} else {
					fprintf(stderr, "Could not open gamecontroller %i: %s\n", i, SDL_GetError());
				}
			}
		}
	}
	writing = false;
	return true;
}

void joystick_step()
{
	if (!writing) { //if we are not already writing, check latch to
		//see if we need to start
		handle_latch(joystick_latch, joystick_clock);
		return;
	}

	//if we have started writing controller data and the latch has dropped,
	// we need to start the next bit
	if (!joystick_latch) {
		//check if clock has changed
		if (joystick_clock != old_clock) {
			if (old_clock) {
				old_clock = joystick_clock;
			} else {  //only write next bit when the new clock is high
				clock_count +=1;
				old_clock = joystick_clock;
				if (clock_count < 16) { // write out the next 15 bits
					joystick1_data = (joy1_mode != NONE) ? (joystick1_state & 1) : 1;
					joystick2_data = (joy2_mode != NONE) ? (joystick2_state & 1) : 1;
					joystick1_state = joystick1_state >> 1;
					joystick2_state = joystick2_state >> 1;
				} else {
					//Done writing controller data
					//reset flag and set count to 0
					writing = false;
					clock_count = 0;
					joystick1_data = (joy1_mode != NONE) ? 0 : 1;
					joystick2_data = (joy2_mode != NONE) ? 0 : 1;
				}
			}
		}
	}



}

bool handle_latch(bool latch, bool clock)
{
	if (latch){
		clock_count = 0;
		//get the 16-representation to put to the VIA
		joystick1_state = get_joystick_state(joystick1, joy1_mode);
		joystick2_state = get_joystick_state(joystick2, joy2_mode);
		//set writing flag to true to signal we will start writing controller data
		writing = true;
		old_clock = clock;
		//preload the first bit onto VIA
		joystick1_data = (joy1_mode != NONE) ? (joystick1_state & 1) : 1;
		joystick2_data = (joy2_mode != NONE) ? (joystick2_state & 1) : 1;
		joystick1_state = joystick1_state >> 1;
		joystick2_state = joystick2_state >> 1;
	}

	return latch;
}

//get current state from SDL controller
//Should replace this with SDL events, so we do not miss inputs when polling
uint16_t get_joystick_state(SDL_GameController *control, enum joy_status mode)
{
	if (mode == NES) {
		bool a_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_A);
		bool b_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_X);
		bool select_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_BACK);
		bool start_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_START);
		bool up_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_UP);
		bool down_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
		bool left_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
		bool right_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

		return
		(!a_pressed) |
		(!b_pressed) << 1 |
		(!select_pressed) << 2 |
		(!start_pressed) << 3 |
		(!up_pressed) << 4 |
		(!down_pressed) << 5 |
		(!left_pressed) << 6 |
		(!right_pressed) << 7 |
		0x0000;
	}
	if (mode == SNES) {
		bool b_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_A);
		bool y_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_X);
		bool select_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_BACK);
		bool start_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_START);
		bool up_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_UP);
		bool down_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
		bool left_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
		bool right_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
		bool a_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_B);
		bool x_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_Y);
		bool l_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
		bool r_pressed = SDL_GameControllerGetButton(control, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);


		return
		(!b_pressed) |
		(!y_pressed) << 1 |
		(!select_pressed) << 2 |
		(!start_pressed) << 3 |
		(!up_pressed) << 4 |
		(!down_pressed) << 5 |
		(!left_pressed) << 6 |
		(!right_pressed) << 7 |
		(!a_pressed) << 8 |
		(!x_pressed) << 9 |
		(!l_pressed) << 10 |
		(!r_pressed) << 11 |
		0xF000;
	}

	return 0xFFFF;
}
