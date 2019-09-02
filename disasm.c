// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include "memory.h"

#define N_SHIFT 7
#define V_SHIFT 6
#define X_SHIFT 5
#define B_SHIFT 4
#define D_SHIFT 3
#define I_SHIFT 2
#define Z_SHIFT 1
#define C_SHIFT 0

enum {
	INSTR_ADC,
	INSTR_AND,
	INSTR_ASL,
	INSTR_BCC,
	INSTR_BCS,
	INSTR_BEQ,
	INSTR_BIT,
	INSTR_BMI,
	INSTR_BNE,
	INSTR_BPL,
	INSTR_BRK,
	INSTR_BVC,
	INSTR_BVS,
	INSTR_CLC,
	INSTR_CLD,
	INSTR_CLI,
	INSTR_CLV,
	INSTR_CMP,
	INSTR_CPX,
	INSTR_CPY,
	INSTR_DEC,
	INSTR_DEX,
	INSTR_DEY,
	INSTR_EOR,
	INSTR_INC,
	INSTR_INX,
	INSTR_INY,
	INSTR_JMP,
	INSTR_JSR,
	INSTR_LDA,
	INSTR_LDX,
	INSTR_LDY,
	INSTR_LSR,
	INSTR_NOP,
	INSTR_ORA,
	INSTR_PHA,
	INSTR_PHP,
	INSTR_PLA,
	INSTR_PLP,
	INSTR_ROL,
	INSTR_ROR,
	INSTR_RTI,
	INSTR_RTS,
	INSTR_SBC,
	INSTR_SEC,
	INSTR_SED,
	INSTR_SEI,
	INSTR_STA,
	INSTR_STX,
	INSTR_STY,
	INSTR_TAX,
	INSTR_TAY,
	INSTR_TSX,
	INSTR_TXA,
	INSTR_TXS,
	INSTR_TYA,
	INSTR_XXX
};

enum {
	ADDMODE_ABS,
	ADDMODE_ABSX,
	ADDMODE_ABSY,
	ADDMODE_ACC,
	ADDMODE_BRA,
	ADDMODE_IMM,
	ADDMODE_IMPL,
	ADDMODE_IND,
	ADDMODE_INDX,
	ADDMODE_INDY,
	ADDMODE_ZP,
	ADDMODE_ZPX,
	ADDMODE_ZPY
};

static int
get_addmode(uint8_t opcode) {
	switch (opcode) {
		case 0x0D:case 0x0E:case 0x20:case 0x2C:case 0x2D:case 0x2E:case 0x4C:case 0x4D:
		case 0x4E:case 0x6D:case 0x6E:case 0x8C:case 0x8D:case 0x8E:case 0xAC:case 0xAD:
		case 0xAE:case 0xCC:case 0xCD:case 0xCE:case 0xEC:case 0xED:case 0xEE:
			return ADDMODE_ABS;

		case 0x1D:case 0x1E:case 0x3D:case 0x3E:case 0x5D:case 0x5E:case 0x7D:case 0x7E:
		case 0x9D:case 0xBC:case 0xBD:case 0xDD:case 0xDE:case 0xFD:case 0xFE:
			return ADDMODE_ABSX;

		case 0x19:case 0x39:case 0x59:case 0x79:case 0x99:case 0xB9:case 0xBE:case 0xD9:
		case 0xF9:
			return ADDMODE_ABSY;

		case 0x0A:case 0x2A:case 0x4A:case 0x6A:
			return ADDMODE_ACC;

		case 0x10:case 0x30:case 0x50:case 0x70:case 0x90:case 0xB0:case 0xD0:case 0xF0:
			return ADDMODE_BRA;

		case 0x09:case 0x29:case 0x49:case 0x69:case 0xA0:case 0xA2:case 0xA9:case 0xC0:
		case 0xC9:case 0xE0:case 0xE9:
			return ADDMODE_IMM;

		case 0x6C:
			return ADDMODE_IND;

		case 0x01:case 0x21:case 0x41:case 0x61:case 0x81:case 0xA1:case 0xC1:case 0xE1:
			return ADDMODE_INDX;

		case 0x11:case 0x31:case 0x51:case 0x71:case 0x91:case 0xB1:case 0xD1:case 0xF1:
			return ADDMODE_INDY;

		case 0x05:case 0x06:case 0x24:case 0x25:case 0x26:case 0x45:case 0x46:case 0x65:
		case 0x66:case 0x84:case 0x85:case 0x86:case 0xA4:case 0xA5:case 0xA6:case 0xC4:
		case 0xC5:case 0xC6:case 0xE4:case 0xE5:case 0xE6:
			return ADDMODE_ZP;

		case 0x15:case 0x16:case 0x35:case 0x36:case 0x55:case 0x56:case 0x75:case 0x76:
		case 0x94:case 0x95:case 0xB4:case 0xB5:case 0xD5:case 0xD6:case 0xF5:case 0xF6:
			return ADDMODE_ZPX;

		case 0x96:
		case 0xB6:
			return ADDMODE_ZPY;
	}
	return ADDMODE_IMPL;
}

static int
get_instr(uint8_t opcode) {
	switch (opcode) {
		case 0x00:
			return INSTR_BRK;
		case 0xEA:
			return INSTR_NOP;

		case 0x8A:
			return INSTR_TXA;
		case 0x98:
			return INSTR_TYA;
		case 0xA8:
			return INSTR_TAY;
		case 0xAA:
			return INSTR_TAX;

		case 0x9A:
			return INSTR_TXS;
		case 0xBA:
			return INSTR_TSX;

		case 0xA1:case 0xA5:case 0xA9:case 0xAD:case 0xB1:case 0xB5:case 0xB9:case 0xBD:
			return INSTR_LDA;
		case 0xA2:case 0xA6:case 0xAE:case 0xB6:case 0xBE:
			return INSTR_LDX;
		case 0xA0:case 0xA4:case 0xAC:case 0xB4:case 0xBC:
			return INSTR_LDY;
		case 0x81:case 0x85:case 0x8D:case 0x91:case 0x95:case 0x99:case 0x9D:
			return INSTR_STA;
		case 0x84:case 0x8C:case 0x94:
			return INSTR_STY;
		case 0x86:case 0x8E:case 0x96:
			return INSTR_STX;

		case 0x21:case 0x25:case 0x29:case 0x2D:case 0x31:case 0x35:case 0x39:case 0x3D:
			return INSTR_AND;
		case 0x01:case 0x05:case 0x09:case 0x0D:case 0x11:case 0x15:case 0x19:case 0x1D:
			return INSTR_ORA;
		case 0x41:case 0x45:case 0x49:case 0x4D:case 0x51:case 0x55:case 0x59:case 0x5D:
			return INSTR_EOR;

		case 0x24:case 0x2C:
			return INSTR_BIT;

		case 0x06:case 0x0A:case 0x0E:case 0x16:case 0x1E:
			return INSTR_ASL;
		case 0x46:case 0x4A:case 0x4E:case 0x56:case 0x5E:
			return INSTR_LSR;
		case 0x26:case 0x2A:case 0x2E:case 0x36:case 0x3E:
			return INSTR_ROL;
		case 0x66:case 0x6A:case 0x6E:case 0x76:case 0x7E:
			return INSTR_ROR;

		case 0xE6:case 0xEE:case 0xF6:case 0xFE:
			return INSTR_INC;
		case 0xC6:case 0xCE:case 0xD6:case 0xDE:
			return INSTR_DEC;
		case 0xE8:
			return INSTR_INX;
		case 0xC8:
			return INSTR_INY;
		case 0xCA:
			return INSTR_DEX;
		case 0x88:
			return INSTR_DEY;

		case 0x61:case 0x65:case 0x69:case 0x6D:case 0x71:case 0x75:case 0x79:case 0x7D:
			return INSTR_ADC;
		case 0xE1:case 0xE5:case 0xE9:case 0xED:case 0xF1:case 0xF5:case 0xF9:case 0xFD:
			return INSTR_SBC;
		case 0xC1:case 0xC5:case 0xC9:case 0xCD:case 0xD1:case 0xD5:case 0xD9:case 0xDD:
			return INSTR_CMP;
		case 0xE0:case 0xE4:case 0xEC:
			return INSTR_CPX;
		case 0xC0:case 0xC4:case 0xCC:
			return INSTR_CPY;

		case 0x08:
			return INSTR_PHP;
		case 0x28:
			return INSTR_PLP;
		case 0x48:
			return INSTR_PHA;
		case 0x68:
			return INSTR_PLA;

		case 0x10:
			return INSTR_BPL;
		case 0x30:
			return INSTR_BMI;
		case 0x50:
			return INSTR_BVC;
		case 0x70:
			return INSTR_BVS;
		case 0x90:
			return INSTR_BCC;
		case 0xB0:
			return INSTR_BCS;
		case 0xD0:
			return INSTR_BNE;
		case 0xF0:
			return INSTR_BEQ;

		case 0x18:
			return INSTR_CLC;
		case 0x58:
			return INSTR_CLI;
		case 0xB8:
			return INSTR_CLV;
		case 0xD8:
			return INSTR_CLD;
		case 0x38:
			return INSTR_SEC;
		case 0x78:
			return INSTR_SEI;
		case 0xF8:
			return INSTR_SED;

		case 0x20:
			return INSTR_JSR;
		case 0x4C:
		case 0x6C:
			return INSTR_JMP;

		case 0x40:
			return INSTR_RTI;
		case 0x60:
			return INSTR_RTS;
	}
	return INSTR_XXX;
}

static int
get_length(int addmode) {
	switch (addmode) {
		case ADDMODE_ACC:
		case ADDMODE_IMPL:
			return 1;
		case ADDMODE_BRA:
		case ADDMODE_IMM:
		case ADDMODE_INDX:
		case ADDMODE_INDY:
		case ADDMODE_ZP:
		case ADDMODE_ZPX:
		case ADDMODE_ZPY:
			return 2;
		case ADDMODE_ABS:
		case ADDMODE_ABSX:
		case ADDMODE_ABSY:
		case ADDMODE_IND:
			return 3;
	}
	return -1; //XXX error
}

static const char *addmode_template[] = {
	/*[ADDMODE_ABS]*/	"$%04x",
	/*[ADDMODE_ABSX]*/	"$%04x,x",
	/*[ADDMODE_ABSY]*/	"$%04x,y",
	/*[ADDMODE_ACC]*/	"a",
	/*[ADDMODE_BRA]*/	"$%02x",
	/*[ADDMODE_IMM]*/	"#$%02x",
	/*[ADDMODE_IMPL]*/	"",
	/*[ADDMODE_IND]*/	"($%04x)",
	/*[ADDMODE_INDX]*/	"($%02x,x)",
	/*[ADDMODE_INDY]*/	"($%02x),y",
	/*[ADDMODE_ZP]*/	"$%02x",
	/*[ADDMODE_ZPX]*/	"$%02x,x",
	/*[ADDMODE_ZPY]*/	"$%02x,y"
};

static const char *mnemo[] = {
	/*[INSTR_ADC]*/ "adc",
	/*[INSTR_AND]*/ "and",
	/*[INSTR_ASL]*/ "asl",
	/*[INSTR_BCC]*/ "bcc",
	/*[INSTR_BCS]*/ "bcs",
	/*[INSTR_BEQ]*/ "beq",
	/*[INSTR_BIT]*/ "bit",
	/*[INSTR_BMI]*/ "bmi",
	/*[INSTR_BNE]*/ "bne",
	/*[INSTR_BPL]*/ "bpl",
	/*[INSTR_BRK]*/ "brk",
	/*[INSTR_BVC]*/ "bvc",
	/*[INSTR_BVS]*/ "bvs",
	/*[INSTR_CLC]*/ "clc",
	/*[INSTR_CLD]*/ "cld",
	/*[INSTR_CLI]*/ "cli",
	/*[INSTR_CLV]*/ "clv",
	/*[INSTR_CMP]*/ "cmp",
	/*[INSTR_CPX]*/ "cpx",
	/*[INSTR_CPY]*/ "cpy",
	/*[INSTR_DEC]*/ "dec",
	/*[INSTR_DEX]*/ "dex",
	/*[INSTR_DEY]*/ "dey",
	/*[INSTR_EOR]*/ "eor",
	/*[INSTR_INC]*/ "inc",
	/*[INSTR_INX]*/ "inx",
	/*[INSTR_INY]*/ "iny",
	/*[INSTR_JMP]*/ "jmp",
	/*[INSTR_JSR]*/ "jsr",
	/*[INSTR_LDA]*/ "lda",
	/*[INSTR_LDX]*/ "ldx",
	/*[INSTR_LDY]*/ "ldy",
	/*[INSTR_LSR]*/ "lsr",
	/*[INSTR_NOP]*/ "nop",
	/*[INSTR_ORA]*/ "ora",
	/*[INSTR_PHA]*/ "pha",
	/*[INSTR_PHP]*/ "php",
	/*[INSTR_PLA]*/ "pla",
	/*[INSTR_PLP]*/ "plp",
	/*[INSTR_ROL]*/ "rol",
	/*[INSTR_ROR]*/ "ror",
	/*[INSTR_RTI]*/ "rti",
	/*[INSTR_RTS]*/ "rts",
	/*[INSTR_SBC]*/ "sbc",
	/*[INSTR_SEC]*/ "sec",
	/*[INSTR_SED]*/ "sed",
	/*[INSTR_SEI]*/ "sei",
	/*[INSTR_STA]*/ "sta",
	/*[INSTR_STX]*/ "stx",
	/*[INSTR_STY]*/ "sty",
	/*[INSTR_TAX]*/ "tax",
	/*[INSTR_TAY]*/ "tay",
	/*[INSTR_TSX]*/ "tsx",
	/*[INSTR_TXA]*/ "txa",
	/*[INSTR_TXS]*/ "txs",
	/*[INSTR_TYA]*/ "tya",
	/*[INSTR_XXX]*/ "???"
};

/*
 * Write an ASCII disassembly of one instruction at "pc"
 * in "RAM" into "line" (max length "max_line"), return
 * number of bytes consumed.
 */
int
disasm(uint16_t pc, uint8_t *RAM, char *line, unsigned int max_line) {
	uint8_t opcode = read6502(pc);
	char line2[8];
	int addmode = get_addmode(opcode);

	if (addmode == ADDMODE_BRA) {
			snprintf(line2, sizeof(line2), "$%02x", pc+2 + (int8_t)read6502(pc+1));
	} else {
		switch (get_length(addmode)) {
			case 1:
				snprintf(line2, sizeof(line2), addmode_template[addmode], 0);
				break;
			case 2:
				snprintf(line2, sizeof(line2), addmode_template[addmode], read6502(pc+1));
				break;
			case 3:
				snprintf(line2, sizeof(line2), addmode_template[addmode], read6502(pc+1) | read6502(pc+2)<<8);
				break;
			default:
				printf("Table error at %s:%d\n", __FILE__, __LINE__);
				exit(1);
		}
	}

	snprintf(line, max_line, "%s %s", mnemo[get_instr(opcode)], line2);
	return get_length(get_addmode(read6502(pc)));
}
