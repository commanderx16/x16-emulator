#pragma once
#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>
#include <stdbool.h>

#define JOY_LATCH_MASK 0x04
#define JOY_CLK_MASK 0x08

#define NUM_JOYSTICKS 4

extern uint8_t Joystick_data;
extern bool Joystick_slots_enabled[NUM_JOYSTICKS];

bool joystick_init(void); //initialize SDL controllers
void joystick_add(int index);
void joystick_remove(int index);

void joystick_button_down(int instance_id, uint8_t button);
void joystick_button_up(int instance_id, uint8_t button);

void joystick_set_latch(bool value);
void joystick_set_clock(bool value);

#endif
