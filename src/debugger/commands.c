#include <stdio.h>
#include <stdlib.h>
#include "../glue.h"
#include "../video.h"
#include "../memory.h"
#include "../console/DT_drawtext.h"
#include "../console/SDL_console.h"
#include "commands.h"
#include "symbols.h"
#include "registers.h"

#define DDUMP_RAM	0
#define DDUMP_VERA	1

extern int dumpmode;
extern int currentBank;
extern int currentData;
extern int currentPCBank;
extern int currentPC;
extern ConsoleInformation *console;
extern int breakpoints[DBG_MAX_BREAKPOINTS];
extern int breakpointsCount;
extern SDL_Renderer *dbgRenderer;
extern void DEBUGsetFont(int fontNumber);
extern int layoutID;
extern void DEBUGupdateLayout(int layoutID);

int isROMdebugAllowed= 0;

command_t cmd_table[] = {
	{ "m", cmd_dump_mem, 0, 0, "m [[bank]address]\ndisplay CPU memory [at the given address]. If not specified the current bank will be used." },
	{ "v", cmd_dump_videomem, 0, 0, "v [address]\ndisplay VIDEO memory [at the given address]." },
	{ "e", cmd_edit_mem, 0, 0, "e address values....\nset memory at address with values" },

	{ "f", cmd_fill_memory, 3, 0, "f address value length [increment:1]\nfill CPU/VIDEO memory" },
	{ "d", cmd_disasm, 1, 0, "f address value length [increment:1]\nfill CPU/VIDEO memory" },
	{ "b", cmd_set_bank, 2, 0, "f address value length [increment:1]\nfillsCPU/VIDEO memory" },
	{ "r", cmd_set_register, 2, 0, "r A|X|Y|PC|SP|P|BKA|BKO|VA|VD0|VD1|VCT value \nset register value" },

	{ "sec", cmd_set_status, 0, 0b000000001, "sec\nset carry" },
	{ "clc", cmd_set_status, 0, 0b100000001, "clc\nclear carry" },

	{ "sez", cmd_set_status, 0, 0b000000010, "sec\nset zero" },
	{ "clz", cmd_set_status, 0, 0b100000010, "clz\nclear zero" },

	{ "sei", cmd_set_status, 0, 0b000000100, "sei\nset interrupts disabled" },
	{ "cli", cmd_set_status, 0, 0b100000100, "cli\nclear interrupts disabled" },

	{ "sed", cmd_set_status, 0, 0b000001000, "sed\nset Decimal mode" },
	{ "cld", cmd_set_status, 0, 0b100001000, "cld\nclear Decimal mode" },

	{ "seb", cmd_set_status, 0, 0b000010000, "seb\nset break" },
	{ "clb", cmd_set_status, 0, 0b100010000, "clb\nclear break" },

	{ "ser", cmd_set_status, 0, 0b000100000, "ser\nset reserved/not used" },
	{ "clr", cmd_set_status, 0, 0b100100000, "clr\nclear reserved/not used" },

	{ "sev", cmd_set_status, 0, 0b001000000, "sev\nset overflow" },
	{ "clv", cmd_set_status, 0, 0b101000000, "clv\nclear overflow" },

	{ "sen", cmd_set_status, 0, 0b010000000, "sen\nset negative" },
	{ "cln", cmd_set_status, 0, 0b110000000, "cln\nclear negative" },

	{ "bpl", cmd_bp_list, 0, 0, "bpl\nlist breakpoints" },
	{ "bp", cmd_bp_add, 1, 0, "bp address\nadd a breakpoint at the specified address" },
	{ "bpc", cmd_bp_clear, 1, 0, "bpc bp_number|*\nclear a specific breakpoint or all" },

	{ "?", cmd_help, 0, 0, "?|help\ndisplay help" },
	{ "help", cmd_help, 0, 0, "?|help\ndisplay help" },

	{ "sym", cmd_sym, 0, 0, "sym [address|symbol]\nlook-up for the address or symbol and display it" },
	{ "symclear", cmd_symclear, 2, 0, "symclear bank [symbol|*]\nclear the \"symbol\" or \"*\" all from the symbol table for specified bank" },
	{ "symload", cmd_symload, 1, 0, "symload [bank] symbolFilename\nload the symbol table [for the specified bank]" },
	{ "symdump", cmd_symsave, 1, 0, "symdump symbolFilename\ndump in a file the symbol dictionary" },

	{ "code", cmd_code, 0, 0, "code\nToggle the display in full page of code/disasm" },
	{ "font", cmd_font, 0, 0, "font [Name|ID [Path]]\nDisplay loaded fonts or Set the debugger font or Load and Set the debugger font" },

	{ "romdebug", cmd_romdebug, 0, 0, "romdebug\ntoggle ROM debug mode to allow editing" },

	{ NULL, NULL }
};

/*
	2000
	102000
	symbol
	10:2000
	10:symbol
*/
int cmd_eval_addr(char *str) {
	int bank= currentPCBank>0 ? currentPCBank : 0;
	char *colon= strchr(str, ':');

	if(colon) {
		bank= (int)strtol(str, NULL, 16);
		str= colon+1;
	}

	int addr= symbol_find_addr(bank, str);
	if(addr == 0xFF000000)
		addr= (int)strtol(str, NULL, 16);
	return (bank << 16) | addr;
}

/* ----------------------------------------------------------------------------
	display CPU memory pane. at the specified bank and address if given
	m [address]
*/
void cmd_dump_mem(int data, int argc, char* argv[]) {
	dumpmode= DDUMP_RAM;
	if(argc == 1)
		return;
	int addr= cmd_eval_addr(argv[1]);
	currentBank= (addr & 0xFF0000) >> 16;
	currentData= addr & 0xFFFF;
}

/* ----------------------------------------------------------------------------
	display VIDEO memory pane. at the specified address if given
	v [address]
*/
void cmd_dump_videomem(int data, int argc, char* argv[]) {
	dumpmode= DDUMP_VERA;
	if(argc == 1)
		return;
	currentData = 0x1FFFF & (int)strtol(argv[1], NULL, 16);
}

/* ----------------------------------------------------------------------------
	set memory at address with values
	e address values...
*/
void cmd_edit_mem(int data, int argc, char* argv[]) {
	int addr= cmd_eval_addr(argv[1]);

	argc-= 2;
	switch(dumpmode) {
		case DDUMP_RAM:
		{
			int value;
			int idx= 2;
			int bank= addr >> 16;
			while(argc--) {
				value= (int)strtol(argv[idx++], NULL, 16);
				addr &= 0xFFFF;
				if(!isValidAddr(bank, addr))
					continue;
				if (addr >= 0xA000) {
					if(addr < 0xC000)
						RAM[0xa000 + (bank << 13) + addr - 0xa000] = value;
					else if(isROMdebugAllowed) {
						ROM[(bank << 14) + addr - 0xc000]= value;
					}
				} else {
					RAM[addr] = value;
				}
				addr++;
			}
			break;
		}

		case DDUMP_VERA:
			break;

	}
}

/* ----------------------------------------------------------------------------
	fill CPU/VIDEO memory depending on the displayed pane.
	f address value length [increment:1]
*/
void cmd_fill_memory(int data, int argc, char* argv[]) {
	int addr= cmd_eval_addr(argv[1]);
	int value= (int)strtol(argv[2], NULL, 16);
	int length= (int)strtol(argv[3], NULL, 16);
	int incr= argc>4 ? (int)strtol(argv[4], NULL, 16) : 1;

	length= length > 0 ? length : 1;

	switch(dumpmode) {

		case DDUMP_RAM:
		{
			int bank= addr >> 16;
			while(length--) {
				addr &= 0xFFFF;
				if(!isValidAddr(bank, addr))
					continue;
				if (addr >= 0xA000) {
					if(addr < 0xC000)
						RAM[0xa000 + (bank << 13) + addr - 0xa000] = value;
				} else {
					RAM[addr] = value;
				}
				addr += incr;
			}
			break;
		}

		case DDUMP_VERA:
			while(length--) {
				addr &= 0x1FFFF;
				video_space_write(addr, value);
				addr += incr;
			}
			break;
	}

}

/* ----------------------------------------------------------------------------
	disasm from specified address
	d [bank]address
*/
void cmd_disasm(int data, int argc, char* argv[]) {
	int addr= cmd_eval_addr(argv[1]);
	currentPCBank= addr >> 16;
	currentPC= addr & 0xFFFF;
}

/* ----------------------------------------------------------------------------
	set the ROM or RAM bank register
	b rom|ram bank
*/
void cmd_set_bank(int data, int argc, char* argv[]) {
	char *type= argv[1];
	int value= (int)strtol(argv[2], NULL, 16) & 0x00FF;

	if(!strcmp(type, "rom")) {
		return memory_set_rom_bank(value);
	}
	if(!strcmp(type, "ram")) {
		return memory_set_ram_bank(value);
	}

	CON_Out(console, "%sERR: Bank is either ROM or RAM%s", DT_color_red, DT_color_default);

}

/* ----------------------------------------------------------------------------
	set the value of the specified register
	r A|X|Y|PC|SP|P|BKA|BKO|VA|VD0|VD1|VCT value
*/
void cmd_set_register(int data, int argc, char* argv[]) {
	char *reg= argv[1];
	int value= (int)strtol(argv[2], NULL, 16);
	int regCode= -1;

	typedef struct {
		char *name;
		int code;
	} TReg;
	TReg registers[] = {
		{ "PC", REG_PC },
		{ "A", REG_A },
		{ "X", REG_X },
		{ "Y", REG_Y },
		{ "SP", REG_SP },
		{ "P", REG_P },
		{ "BKA", REG_BKA },
		{ "BKO", REG_BKO },
		{ "VA", REG_VA },
		{ "VD0", REG_VD0 },
		{ "VD1", REG_VD1 },
		{ "VCT", REG_VCT },
		{ NULL, 0 }
	};

	for (int idx= 0; registers[idx].name; idx++) {
		if(!strcasecmp(registers[idx].name, reg)) {
			regCode= registers[idx].code;
			break;
		}
	}

	switch(regCode) {
		case REG_PC:		pc= value & 0xFFFF; break;
		case REG_A:			a= value & 0x00FF; break;
		case REG_X:			x= value & 0x00FF; break;
		case REG_Y:			y= value & 0x00FF; break;
		case REG_SP:		sp= value & 0x00FF; break;
		case REG_P:			status= value & 0x00FF; break;
		case REG_BKA:		memory_set_ram_bank(value & 0x00FF); break;
		case REG_BKO:		memory_set_rom_bank(value & 0x00FF); break;
		case REG_VA:		video_write(0, value & 0x00FF);
							video_write(1, (value>>8) & 0xFF);
							video_write(2, (value>>16) & 0x00FF); break;
		case REG_VD0:		video_write(3, value & 0x00FF); break;
		case REG_VD1:		video_write(4, value & 0x00FF); break;
		case REG_VCT:		video_write(5, value & 0x00FF); break;
		default:
		{
			CON_Out(console, "%sERR: Unknown Register '%s' %s", DT_color_red, reg, DT_color_default);
			char buffer[64]= "";
			for(int idx= 0; registers[idx].name; idx++) {
				if(idx)
					strncat(buffer, ", ", sizeof(buffer)-1);
				strncat(buffer, registers[idx].name, sizeof(buffer)-1);
			}
			CON_Out(console, "Valid Registers are %s", buffer);
		}
	}

}

/* ----------------------------------------------------------------------------
	set/clear a flag in Status Register
	se? cl?
*/
void cmd_set_status(int data, int argc, char* argv[]) {
	int mask= data & 0xFF;
	if(mask == data)
		status|= mask;
	else
		status&= mask ^ 0xFF;

}

/* ----------------------------------------------------------------------------
	display help
	?|help [command]
*/
void cmd_help(int data, int argc, char* argv[]) {
	char buffer[CON_CHARS_PER_LINE+1]= "\0";
	char *pBuffer= buffer;

	for(int idx= 0; cmd_table[idx].name; idx++) {
		if(argc == 2) {
			if(!stricmp(argv[1], cmd_table[idx].name)) {
				pBuffer= cmd_table[idx].help;
				break;
			}
		} else {
			if(strlen(pBuffer) + strlen(cmd_table[idx].name) > sizeof(buffer)) {
				CON_Out(console, pBuffer);
				pBuffer[0]= '\0';
			}
			strncat(pBuffer, cmd_table[idx].name, sizeof(buffer)-1);
			strncat(pBuffer, " ", sizeof(buffer)-1);
		}
	}

	CON_Out(console, pBuffer);
}

/* ----------------------------------------------------------------------------
	list breakpoints
	bpl
*/
void cmd_bp_list(int data, int argc, char* argv[]) {
	if(breakpointsCount == 0) {
		CON_Out(console, "There is no current breakpoints");
		return;
	}
	for(int idx= 0; idx < breakpointsCount; idx++) {
		CON_Out(console, "%02d - %X", idx, breakpoints[idx]);
	}
}

/* ----------------------------------------------------------------------------
	add breakpoint
	bp address
*/
void cmd_bp_add(int data, int argc, char* argv[]) {
	int addr= cmd_eval_addr(argv[1]);
	int bank= addr >> 16;

	if(breakpointsCount >= DBG_MAX_BREAKPOINTS)
		return CON_Out(console, "You have reached the max number (%d) of breakpoints allowed", DBG_MAX_BREAKPOINTS);

	if(!isValidAddr(bank, addr))
		return CON_Out(console, "%sERR: Invalid address%s", DT_color_red, DT_color_default);

	breakpointsCount++;
	breakpoints[breakpointsCount-1]= addr;
}

/* ----------------------------------------------------------------------------
	clear a specific breakpoint or all
	bpc bp_number|*
*/
void cmd_bp_clear(int data, int argc, char* argv[]) {
	if(!strcmp(argv[1],"*")) {
		breakpointsCount= 0;
		return;
	}

	int bpIdx= (int)strtol(argv[1], NULL, 16);
	if(bpIdx > breakpointsCount-1)
		return;

	for(int idx= bpIdx; idx < breakpointsCount-1; idx++) {
		breakpoints[idx]= breakpoints[idx+1];
	}
	breakpointsCount--;
}

/* ----------------------------------------------------------------------------
	look-up for the address or symbol and display it
	sym [address|symbol]
*/
void cmd_sym(int data, int argc, char* argv[]) {

	const char *addresses= symbol_lookup(0, argv[1]);
	if(addresses)
		CON_Out(console, addresses);
	else
		CON_Out(console, "symbol \"%s\" not found", argv[1]);

	// char label[15];
	// TSymbolVolume *vol;
	// int hasFound= 0;

	// strncpy(label, argv[1], 14);
	// label[14]= 0;

	// for(int idx= 0; (vol= symbol_get_volume(idx)); idx++) {
	// 	int addr= symbol_find_addr(vol->bank, label);
	// 	if(addr != 0xFF000000) {
	// 		hasFound= 1;
	// 		CON_Out(console, "%s : %06X", label, (vol->bank << 16) | addr);
	// 	}
	// }
	// if(!hasFound)
	// 	CON_Out(console, "symbol \"%s\" not found", label);
}

/* ----------------------------------------------------------------------------
	clear the "symbol" or "*" all from the symbol table for specified bank
	symclear bank [symbol|*]
*/
void cmd_symclear(int data, int argc, char* argv[]) {
	int bank= (int)strtol(argv[1], NULL, 16);
	int count= -1;

	if(!strcmp("*", argv[2]))
		count= symbol_clear_volume(bank);

	if(count<0)
		CON_Out(console, "nothing to delete from bank %02X", bank);
	else
		CON_Out(console, "%d symbol(s) were deleted from bank %02X", count, bank);
}

/* ----------------------------------------------------------------------------
	load the symbol table [for the specified bank]
	symload [bank] symbolFilename
*/
void cmd_symload(int data, int argc, char* argv[]) {
	int bank= -1;
	char *filename= NULL;
	int lineCount= 0;
	int addedCount= 0;
	int dupCount= 0;

	if(argc>2) {
		bank= (int)strtol(argv[1], NULL, 16);
		filename= argv[2];
	} else
		filename= argv[1];

	int err= symbol_load(filename, bank, &addedCount, &dupCount, &lineCount);

	switch(err) {
		case CantOpen:
			CON_Out(console, "Unable to open file \"%s\"", filename);
			break;

		case SynErr:
			CON_Out(console, "Syntax Error in line %d", lineCount);
			break;

		default:
			if(bank<0)
				CON_Out(console, "%d symbols were added. %d duplicates were updated", addedCount, dupCount);
			else
				CON_Out(console, "bank %x: %d symbols were added. %d duplicates were updated", bank, addedCount, dupCount);
	}

}

/* ----------------------------------------------------------------------------
	load the symbol table [for the specified bank]
	symload [bank] symbolFilename
*/
void cmd_symsave(int data, int argc, char* argv[]) {
	symbol_dump(argv[1]);
}

/* ----------------------------------------------------------------------------
	display the code in full page
	code
*/
void cmd_code(int data, int argc, char* argv[]) {
	DEBUGupdateLayout(layoutID==0 ? 1 : 0);
}

/* ----------------------------------------------------------------------------
	Display loaded fonts or Set the debugger font or Load and Set the debugger font
	font [Name|ID [Path]]
*/
void cmd_font(int data, int argc, char* argv[]) {

	switch(argc) {
		case 1:
		{
			BitFont *font;
			for(int idx= 0; (font= DT_FontPointer(idx)); idx++)
				CON_Out(console, "[%02d] H:%02d W:%02d N:%s", font->fontNumber, font->charHeight, font->charWidth, font->fontName);
			break;
		}
		case 2:
		{
			int fontNum= DT_FindFontID(argv[1]);
			if(fontNum >= 0)
				DEBUGsetFont(fontNum);
			else
				CON_Out(console, "%sNo such Font%s", DT_color_red, DT_color_default);
			break;
		}
		case 3:
		{
			int fontNum= DT_LoadFont(dbgRenderer, argv[2], 1);
			if(fontNum>=0) {
				DT_SetFontName(fontNum, argv[1]);
				DEBUGsetFont(fontNum);
			} else
				CON_Out(console, "%sUnable to load this font: %s%s", DT_color_red, SDL_GetError(), DT_color_default);
			break;
		}
	}

}

/* ----------------------------------------------------------------------------
	toggle ROM debug mode to allow editing
	romdebug
*/
void cmd_romdebug(int data, int argc, char* argv[]) {
	isROMdebugAllowed= 1 - isROMdebugAllowed;
	if(isROMdebugAllowed)
		CON_Out(console, "%sCAUTION%s! ROM can be edited now", DT_color_red, DT_color_default);
	else
		CON_Out(console, "ROM is read-only now");
}

