#include "joystick.h"

#include <SDL.h>
#include <stdio.h>

struct joystick_info {
	int                 instance_id;
	SDL_GameController *controller;
	uint16_t            button_mask;
	uint16_t            shift_mask;
};

static const uint16_t button_map[SDL_CONTROLLER_BUTTON_MAX] = {
    1 << 0,  //SDL_CONTROLLER_BUTTON_A,
    1 << 8,  //SDL_CONTROLLER_BUTTON_B,
    1 << 1,  //SDL_CONTROLLER_BUTTON_X,
    1 << 9,  //SDL_CONTROLLER_BUTTON_Y,
    1 << 2,  //SDL_CONTROLLER_BUTTON_BACK,
    0,       //SDL_CONTROLLER_BUTTON_GUIDE,
    1 << 3,  //SDL_CONTROLLER_BUTTON_START,
    0,       //SDL_CONTROLLER_BUTTON_LEFTSTICK,
    0,       //SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    1 << 10, //SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    1 << 11, //SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    1 << 4,  //SDL_CONTROLLER_BUTTON_DPAD_UP,
    1 << 5,  //SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    1 << 6,  //SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    1 << 7,  //SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
};

static struct joystick_info *Joystick_controllers     = NULL;
static int                   Num_joystick_controllers = 0;

static void
resize_joystick_controllers(int new_size)
{
	if (new_size == 0) {
		free(Joystick_controllers);
		Joystick_controllers     = NULL;
		Num_joystick_controllers = 0;
		return;
	}

	struct joystick_info *old_controllers = Joystick_controllers;
	Joystick_controllers                  = (struct joystick_info *)malloc(sizeof(struct joystick_info) * new_size);

	int min_size = new_size < Num_joystick_controllers ? new_size : Num_joystick_controllers;
	if (min_size > 0) {
		memcpy(Joystick_controllers, old_controllers, sizeof(struct joystick_info) * min_size);
		free(old_controllers);
	}

	for (int i = min_size; i < new_size; ++i) {
		Joystick_controllers[i].instance_id = -1;
		Joystick_controllers[i].controller  = NULL;
		Joystick_controllers[i].button_mask = 0xffff;
		Joystick_controllers[i].shift_mask  = 0;
	}
}

static void
add_joystick_controller(struct joystick_info *info)
{
	int i;
	for (i = 0; i < Num_joystick_controllers; ++i) {
		if (Joystick_controllers[i].instance_id == -1) {
			memcpy(&Joystick_controllers[i], info, sizeof(struct joystick_info));
			return;
		}
	}

	i = Num_joystick_controllers;
	resize_joystick_controllers(Num_joystick_controllers << 1);

	memcpy(&Joystick_controllers[i], info, sizeof(struct joystick_info));
}

static void
remove_joystick_controller(int instance_id)
{
	for (int i = 0; i < Num_joystick_controllers; ++i) {
		if (Joystick_controllers[i].instance_id == instance_id) {
			Joystick_controllers[i].instance_id = -1;
			Joystick_controllers[i].controller  = NULL;
			return;
		}
	}
}

static struct joystick_info *
find_joystick_controller(int instance_id)
{
	for (int i = 0; i < Num_joystick_controllers; ++i) {
		if (Joystick_controllers[i].instance_id == instance_id) {
			return &Joystick_controllers[i];
		}
	}

	return NULL;
}

bool Joystick_slots_enabled[NUM_JOYSTICKS] = {false, false, false, false};
static int Joystick_slots[NUM_JOYSTICKS];

static bool Joystick_latch = false;
uint8_t Joystick_data  = 0;

bool
joystick_init(void)
{
	for (int i = 0; i < NUM_JOYSTICKS; ++i) {
		Joystick_slots[i] = -1;
	}

	const int num_joysticks = SDL_NumJoysticks();

	Num_joystick_controllers = num_joysticks > 16 ? num_joysticks : 16;
	Joystick_controllers     = malloc(sizeof(struct joystick_info) * Num_joystick_controllers);

	for (int i = 0; i < Num_joystick_controllers; ++i) {
		Joystick_controllers[i].instance_id = -1;
		Joystick_controllers[i].controller  = NULL;
		Joystick_controllers[i].button_mask = 0xffff;
		Joystick_controllers[i].shift_mask  = 0;
	}

	for (int i = 0; i < num_joysticks; ++i) {
		joystick_add(i);
	}

	return true;
}

void
joystick_add(int index)
{
	if (!SDL_IsGameController(index)) {
		return;
	}

	SDL_GameController *controller = SDL_GameControllerOpen(index);
	if (controller == NULL) {
		fprintf(stderr, "Could not open controller %d: %s\n", index, SDL_GetError());
		return;
	}

	SDL_JoystickID instance_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
	bool           exists      = false;
	for (int i = 0; i < NUM_JOYSTICKS; ++i) {
		if (!Joystick_slots_enabled[i]) {
			continue;
		}

		if (Joystick_slots[i] == instance_id) {
			exists = true;
			break;
		}
	}

	if (!exists) {
		for (int i = 0; i < NUM_JOYSTICKS; ++i) {
			if (!Joystick_slots_enabled[i]) {
				continue;
			}

			if (Joystick_slots[i] == -1) {
				Joystick_slots[i] = instance_id;
				break;
			}
		}

		struct joystick_info new_info;
		new_info.instance_id = instance_id;
		new_info.controller  = controller;
		new_info.button_mask = 0xffff;
		new_info.shift_mask  = 0;
		add_joystick_controller(&new_info);
	}
}

void
joystick_remove(int instance_id)
{
	for (int i = 0; i < NUM_JOYSTICKS; ++i) {
		if (Joystick_slots[i] == instance_id) {
			Joystick_slots[i] = -1;
			break;
		}
	}

	SDL_GameController *controller = SDL_GameControllerFromInstanceID(instance_id);
	if (controller == NULL) {
		fprintf(stderr, "Could not find controller from instance_id %d: %s\n", instance_id, SDL_GetError());
	} else {
		SDL_GameControllerClose(controller);
		remove_joystick_controller(instance_id);
	}
}

void
joystick_button_down(int instance_id, uint8_t button)
{
	struct joystick_info *joy = find_joystick_controller(instance_id);
	if (joy != NULL) {
		joy->button_mask &= ~(button_map[button]);
	}
}

void
joystick_button_up(int instance_id, uint8_t button)
{
	struct joystick_info *joy = find_joystick_controller(instance_id);
	if (joy != NULL) {
		joy->button_mask |= button_map[button];
	}
}

static void
do_shift()
{
	for (int i = 0; i < NUM_JOYSTICKS; ++i) {
		if (Joystick_slots[i] >= 0) {
			struct joystick_info *joy = find_joystick_controller(Joystick_slots[i]);
			if (joy != NULL) {
				Joystick_data |= ((joy->shift_mask & 1) ? (0x80 >> i) : 0);
				joy->shift_mask >>= 1;
			} else {
				Joystick_data |= 0x80 >> i;
			}
		} else {
			Joystick_data |= 0x80 >> i;
		}
	}
}

void
joystick_set_latch(bool value)
{
	Joystick_latch = value;
	if (value) {
		for (int i = 0; i < Num_joystick_controllers; ++i) {
			if (Joystick_controllers[i].instance_id != -1) {
				Joystick_controllers[i].shift_mask = Joystick_controllers[i].button_mask | 0xF000;
			}
		}
		do_shift();
	}
}

void
joystick_set_clock(bool value)
{
	if (!Joystick_latch && value) {
		Joystick_data = 0;
		do_shift();
	}
}
