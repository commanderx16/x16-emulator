// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "memory.h"

#include "cpu/mnemonics.h"				// Automatically generated mnemonic table.

// *******************************************************************************************
//
//		Disassemble a single 65C02 instruction into buffer. Returns the length of the
//		instruction in total in bytes.
//
// *******************************************************************************************

int disasm(uint16_t pc, uint8_t *RAM, char *line, unsigned int max_line) {
	uint8_t opcode = read6502(pc);
	char *mnemonic = mnemonics[opcode];

	//
	//		Test for branches, relative address. These are BRA ($80) and 
	//		$10,$30,$50,$70,$90,$B0,$D0,$F0.
	//
	//
	int isBranch = (opcode == 0x80) || ((opcode & 0x1F) == 0x10);
	int length = 1;							

	strncpy(line,mnemonic,max_line);

	if (strstr(line,"%02x")) {
		length = 2;
		if (isBranch) {
			snprintf(line, max_line, mnemonic, pc+2 + (int8_t)read6502(pc+1));
		} else {
			snprintf(line, max_line, mnemonic, read6502(pc+1));
		}
	}
	if (strstr(line,"%04x")) {
		length = 3;
		snprintf(line, max_line, mnemonic, read6502(pc+1) | read6502(pc+2)<<8);
	}	 
	return length;
}
