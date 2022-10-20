// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

// use w65c02sce as core

#ifndef _65C02_H_
#define _65C02_H_

#include <stdint.h>

void cpu_init(void);
void cpu_reset(void);
void cpu_break(void);
void cpu_nmi(void);
void cpu_irq(int irq_high);
unsigned long cpu_step(void);
/* runs CPU for at least c cycles, returns true number of cycles run */
unsigned long cpu_run(unsigned long c);
unsigned long cpu_clock(void);
unsigned long cpu_instruction_count(void);
/* stall for the given number of cycles */
void cpu_stall(unsigned long c);

uint8_t cpu_get_a(void);
uint8_t cpu_get_x(void);
uint8_t cpu_get_y(void);
uint8_t cpu_get_sp(void);
uint8_t cpu_get_status(void);
uint16_t cpu_get_pc(void);

void cpu_set_a(uint8_t a);
void cpu_set_x(uint8_t x);
void cpu_set_y(uint8_t y);
void cpu_set_sp(uint8_t sp);
void cpu_set_status(uint8_t status);
void cpu_set_pc(uint16_t pc);

#endif
