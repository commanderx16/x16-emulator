// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

// use w65c02s.h as core

#include <stddef.h>

#define W65C02S_IMPL 1
#define W65C02S_LINK 1
#define W65C02S_COARSE 1
#define W65C02S_HOOK_STP 1
#include "w65c02s.h"

static struct w65c02s_cpu cpu;

extern bool handle_cpu_stop(void);

void cpu_init(void) {
    w65c02s_init(&cpu, NULL, NULL, NULL);
    w65c02s_hook_stp(&cpu, &handle_cpu_stop);
}

void cpu_reset(void) {
    w65c02s_reset(&cpu);
}

void cpu_break(void) {
    w65c02s_break(&cpu);
}

void cpu_nmi(void) {
    w65c02s_nmi(&cpu);
}

void cpu_irq(int irq_high) {
    if (irq_high)
        w65c02s_irq(&cpu);
    else
        w65c02s_irq_cancel(&cpu);
}

unsigned long cpu_step(void) {
    return w65c02s_step_instruction(&cpu);
}

/* runs CPU for at least c cycles, returns true number of cycles run */
unsigned long cpu_run(unsigned long c) {
    return w65c02s_run_cycles(&cpu, c);
}

unsigned long cpu_clock(void) {
    return w65c02s_get_cycle_count(&cpu);
}

unsigned long cpu_instruction_count(void) {
    return w65c02s_get_instruction_count(&cpu);
}

/* stall for the given number of cycles */
void cpu_stall(unsigned long c) {
    w65c02s_stall(&cpu, c);
}

uint8_t cpu_get_a(void) {
    return w65c02s_reg_get_a(&cpu);
}

uint8_t cpu_get_x(void) {
    return w65c02s_reg_get_x(&cpu);
}

uint8_t cpu_get_y(void) {
    return w65c02s_reg_get_y(&cpu);
}

uint8_t cpu_get_sp(void) {
    return w65c02s_reg_get_s(&cpu);
}

uint8_t cpu_get_status(void) {
    return w65c02s_reg_get_p(&cpu);
}

uint16_t cpu_get_pc(void) {
    return w65c02s_reg_get_pc(&cpu);
}

void cpu_set_a(uint8_t a) {
    w65c02s_reg_set_a(&cpu, a);
}

void cpu_set_x(uint8_t x) {
    w65c02s_reg_set_x(&cpu, x);
}

void cpu_set_y(uint8_t y) {
    w65c02s_reg_set_y(&cpu, y);
}

void cpu_set_sp(uint8_t sp) {
    w65c02s_reg_set_s(&cpu, sp);
}

void cpu_set_status(uint8_t status) {
    w65c02s_reg_set_p(&cpu, status);
}

void cpu_set_pc(uint16_t pc) {
    w65c02s_reg_set_pc(&cpu, pc);
}
