/**********************************************/
// File     :     joystick.h
// Author   :     John Bliss
// Date     :     September 27th 2019
/**********************************************/

#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL.h>

#include "glue.h"
#include "via.h"

#define JOY_LATCH_MASK 0x08
#define JOY_DATA1_MASK 0x10
#define JOY_CLK_MASK 0x20
#define JOY_DATA2_MASK 0x40

enum joy_status { NES=0, NONE=1, SNES=0xF };
extern enum joy_status joy1_mode;
extern enum joy_status joy2_mode;
extern bool joystick_latch, joystick_clock;
extern bool joystick1_data, joystick2_data;


bool joystick_init(); //initialize SDL controllers

void joystick_step(); //do next step for handling joysticks

bool handle_latch(bool latch, bool clock);  //used internally to check when to
											//  write to VIA

					//Used to get the 16-bit data needed to send
uint16_t get_joystick_state(SDL_GameController *control, enum joy_status mode);

#endif
