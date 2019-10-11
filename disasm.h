// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef _DISASM_H_
#define _DISASM_H_

#include <stdint.h>

int disasm(uint16_t pc, uint8_t *RAM, char *line, unsigned int max_line, bool debugOn, uint8_t bank);

#endif
