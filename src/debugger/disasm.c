// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "../memory.h"
#include "symbols.h"

#include "../cpu/mnemonics.h"				// Automatically generated mnemonic table.

// *******************************************************************************************
//
//		Disassemble a single 65C02 instruction into buffer. Returns the length of the
//		instruction in total in bytes.
//
// *******************************************************************************************

int disasm(uint16_t pc, uint8_t *RAM, char *line, unsigned int max_line, uint8_t bank) {
	uint8_t opcode = real_read6502(pc, true, bank);
	char const *mnemonic = mnemonics[opcode];
	int addr;
	char *fmt;

	//
	//		Test for branches, relative address. These are BRA ($80) and
	//		$10,$30,$50,$70,$90,$B0,$D0,$F0.
	//
	//
	int isBranch = (opcode == 0x80) || ((opcode & 0x1F) == 0x10);
	//
	//		Ditto bbr and bbs, the "zero-page, relative" ops.
	//		$0F,$1F,$2F,$3F,$4F,$5F,$6F,$7F,$8F,$9F,$AF,$BF,$CF,$DF,$EF,$FF
	//
	int isZprel  = (opcode & 0x0F) == 0x0F;

	int length   = 1;
	strncpy(line, mnemonic, max_line);

	if (isZprel) {
		snprintf(line, max_line, mnemonic, real_read6502(pc + 1, true, bank), pc + 3 + (int8_t)real_read6502(pc + 2, true, bank));
		length = 3;
	} else {
		if (strstr(line, "%02x")) {
			length = 2;
			if (isBranch) {
				snprintf(line, max_line, mnemonic, pc + 2 + (int8_t)real_read6502(pc + 1, true, bank));
			} else {
				snprintf(line, max_line, mnemonic, real_read6502(pc + 1, true, bank));
			}
		}
		if ((fmt= strstr(line, "%04x"))) {
			length = 3;
			addr= real_read6502(pc + 1, true, bank) | real_read6502(pc + 2, true, bank) << 8;
			char *label= symbol_find_label(bank, addr);
			if(label) {
				char buffer[48];
				strcpy(buffer, line);
				buffer[fmt-line-1]= '\0';
				strcat(buffer, "%s");
				strcat(buffer, fmt+4);
				snprintf(line, max_line, buffer, label);
			} else
				snprintf(line, max_line, mnemonic, addr);
		}
	}
	return length;
}
