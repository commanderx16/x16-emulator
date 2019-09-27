/**********************************************/
// File     :     controller.h
// Author   :     John Bliss
// Date     :     September 27th 2019
/**********************************************/

#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

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

enum joy_status{NES=0, NONE=1, SNES=0xF};
extern enum joy_status joy1_mode;
extern enum joy_status joy2_mode;
extern bool controller_latch, controller_clock;
extern bool controller_data1, controller_data2;


bool controller_init(); //initialize SDL controllers

void controller_step(); //do next step for handling controllers

bool handle_latch(bool latch, bool clock);  //used internally to check when to
                                            //  write to VIA

                              //Used to get the 16-bit data needed to send
uint16_t get_controller_state(SDL_GameController *control);

#endif
