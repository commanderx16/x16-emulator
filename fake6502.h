// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _FAKE6502_H_
#define _FAKE6502_H_

#include <stdint.h>

extern void reset6502();
extern void step6502();
extern void exec6502(uint32_t tickcount);
extern void irq6502();
extern uint32_t clockticks6502;

#endif
