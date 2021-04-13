#include <stdio.h>
#include <stdlib.h>
#include "../glue.h"
#include "../video.h"
#include "../memory.h"
#include "../cpu/fake6502.h"
#include "../console/DT_drawtext.h"
#include "../console/SDL_console.h"
#include "commands.h"
#include "../iniparser/iniparser.h"
#include "symbols.h"
#include "registers.h"
#include "version.h"
#include "../version.h"
#include "breakpoints.h"

#define DDUMP_RAM	0
#define DDUMP_VERA	1

extern int dumpmode;
extern int currentBank;
extern int currentData;
extern int currentPCBank;
extern int currentPC;
extern ConsoleInformation *console;

extern TBreakpoint breakpoints[DBG_MAX_BREAKPOINTS];
extern int breakpointsCount;

extern SDL_Renderer *dbgRenderer;
extern void DEBUGsetFont(int fontNumber);
extern int layoutID;
extern void DEBUGupdateLayout(int layoutID);

// VARS
int var_isROMdebugAllowed= 0;
int var_BPonBRK= 1;

command_t cmd_table[] = {
	{ "m", cmd_dump_mem, 0, 0, "m [[bank]address]\ndisplay CPU memory [at the given address]. If not specified the current bank will be used." },
	{ "v", cmd_dump_videomem, 0, 0, "v [address]\ndisplay VIDEO memory [at the given address]." },
	{ "e", cmd_edit_mem, 0, 0, "e address values....\nset memory at address with values" },

	{ "f", cmd_fill_memory, 3, 0, "f address value length [increment:1]\nfill CPU/VIDEO memory" },
	{ "sh", cmd_search_memory, 2, 0, "sh address,length|start.end value0 value1 ...\nsearch CPU/VIDEO memory in range (addr,len or start.end) for bytes" },

	{ "d", cmd_disasm, 1, 0, "d address\ndisasm from address" },
	{ "b", cmd_set_bank, 2, 0, "b rom|ram bankNumber\nset RAM or ROM bank" },
	{ "r", cmd_set_register, 2, 0, "r A|X|Y|PC|SP|P|BKA|BKO|VA|VD0|VD1|VCT value \nset register value" },

	{ "sec", cmd_set_status, 0, 0x01, "sec\nset Carry" },
	{ "clc", cmd_set_status, 0, 0x01 | 0x8000, "clc\nclear Carry" },

	{ "sez", cmd_set_status, 0, 0x02, "sec\nset Zero" },
	{ "clz", cmd_set_status, 0, 0x02 | 0x8000, "clz\nclear Zero" },

	{ "sei", cmd_set_status, 0, 0x04, "sei\nset Interrupts disabled" },
	{ "cli", cmd_set_status, 0, 0x04 | 0x8000, "cli\nclear Interrupts disabled" },

	{ "sed", cmd_set_status, 0, 0x08, "sed\nset Decimal mode" },
	{ "cld", cmd_set_status, 0, 0x08 | 0x8000, "cld\nclear Decimal mode" },

	{ "seb", cmd_set_status, 0, 0x10, "seb\nset Break" },
	{ "clb", cmd_set_status, 0, 0x10 | 0x8000, "clb\nclear Break" },

	{ "ser", cmd_set_status, 0, 0x20, "ser\nset Reserved/not used" },
	{ "clr", cmd_set_status, 0, 0x20 | 0x8000, "clr\nclear Reserved/not used" },

	{ "sev", cmd_set_status, 0, 0x40, "sev\nset oVerflow" },
	{ "clv", cmd_set_status, 0, 0x40 | 0x8000, "clv\nclear oVerflow" },

	{ "sen", cmd_set_status, 0, 0x80, "sen\nset Negative" },
	{ "cln", cmd_set_status, 0, 0x80 | 0x8000, "cln\nclear Negative" },

	{ "bpl", cmd_bp_list, 0, 0, "bpl\nlist breakpoints" },
	{ "bp", cmd_bp_add, 1, 0, "bp address\nadd a breakpoint at the specified address" },
	{ "bpm", cmd_bpm_add, 1, BPT_MEM, "bpm address\nadd a memory access breakpoint at the specified address" },
	{ "bpmr", cmd_bpm_add, 1, BPT_RMEM, "bpmr address\nadd a memory read breakpoint at the specified address" },
	{ "bpmw", cmd_bpm_add, 1, BPT_WMEM, "bpmw address\nadd a memory write breakpoint at the specified address" },
	{ "bpc", cmd_bp_clear, 1, 0, "bpc bp_number|*\nclear a specific breakpoint or all" },

	{ "?", cmd_help, 0, 0, "?|help\ndisplay help" },
	{ "help", cmd_help, 0, 0, "?|help\ndisplay help" },

	{ "sym", cmd_sym, 1, 0, "sym address|symbol\nlook-up for the address or symbol and display it" },
	{ "symclear", cmd_symclear, 2, 0, "symclear bank [symbol|*]\nclear the \"symbol\" or \"*\" all from the symbol table for specified bank" },
	{ "symload", cmd_symload, 1, 0, "symload [bank] symbolFilename\nload the symbol table [for the specified bank]" },
	{ "symdump", cmd_symsave, 1, 0, "symdump symbolFilename\ndump in a file the symbol dictionary" },

	{ "code", cmd_code, 0, 0, "code\nToggle the display in full page of code/disasm" },
	{ "data", cmd_data, 0, 0, "data\nToggle the display in full page of data" },
	{ "font", cmd_font, 0, 0, "font [Name|ID [Path]]\nDisplay loaded fonts or Set the debugger font or Load and Set the debugger font" },

	// { "romdebug", cmd_romdebug, 0, 0, "romdebug\ntoggle ROM debug mode to allow editing" },
	{ "info", cmd_info, 0, 0, "info\ndisplay X16 emulator & debugger info" },
	{ "ticks", cmd_ticks, 0, 0, "ticks\ndisplay CPU clock ticks counter" },
	{ "time", cmd_time, 1, 0, "time\nstart timer" },
	{ "timelog", cmd_time, 1, 1, "timelog\nlog timer" },
	{ "timeend", cmd_time, 1, 2, "timeend\nend timer" },

	{ "load", cmd_load, 2, 0, "load file addr [[from:]len]\nload binary file in memory" },
	{ "var", cmd_var, 0, 0, "var name [value] \nset or get value for settings var" },

	{ NULL, NULL, 0, 0, NULL }
};

/*

*/
void commands_init() {
	symbol_init();

	var_define("ROM_DEBUG", "allow to modify ROM", &var_isROMdebugAllowed);
	var_define("BP_ON_BRK", "allow to see BRK as debugger breakpoint", &var_BPonBRK);

}

/*

*/
void commands_free() {
	symbol_free();
}


/*
	2000		2000
	102000		102000
	symbol		5C598
	10:2000		102000
	10:022000	102000
	10:symbol	102000
*/
unsigned int cmd_eval_addr(char *str) {
	int bank= -1;
	char *colon= strchr(str, ':');
	char *end;
	const char *strEnd;

	if(colon) {
		bank= (int)strtol(str, NULL, 16);
		str= colon+1;
	}
	strEnd= str + strlen(str);
	unsigned int addr= (int)strtol(str, &end, 16);

	// number ?
	if(end == strEnd) {
		if(bank>=0)
			return (addr & 0xFFFF) | bank << 16;

		if(addr <= 0xFFFF)
			addr|= currentPCBank << 16;

		return addr;
	}

	if(bank<0) {
		const char *addresses= symbol_lookup(str);
		if(!addresses)
			return SYMBOL_NOT_FOUND;
		addr= (int)strtol(addresses, &end, 16);
		// return addr if no duplicates
		if(end == (addresses+strlen(addresses))) {
			return addr;
		}
		return SYMBOL_NOT_FOUND;
	}

	addr= symbol_find_addr(bank, str);
	return addr == SYMBOL_NOT_FOUND ? SYMBOL_NOT_FOUND : addr;

}

/* ----------------------------------------------------------------------------
	eval range string
	"<symbol|addr>,<length>"
	"<start>.<end>"
*/
int cmd_get_range(char *parm, unsigned int *start, unsigned int *end, int *len) {
	char *sep;

	sep= strchr(parm, ',');
	if(sep) {
		*sep= '\0';
		sep++;
		*start= cmd_eval_addr(parm);
		if(*start == SYMBOL_NOT_FOUND) {
			CON_Out(console, "symbol \"%s\" not found", parm);
			return -1;
		}

		*len= (int)strtol(sep, NULL, 16);
		if(*len>0) {
			*end= *start + *len;
			return 0;
		}
	}
	sep= strchr(parm, '.');
	if(sep) {
		*sep= '\0';
		sep++;
		*start= cmd_eval_addr(parm);
		if(*start == SYMBOL_NOT_FOUND) {
			CON_Out(console, "symbol \"%s\" not found", parm);
			return -1;
		}

		*end= cmd_eval_addr(sep);
		if(*end == SYMBOL_NOT_FOUND) {
			CON_Out(console, "symbol \"%s\" not found", sep);
			return -1;
		}

		if(*end>*start) {
			*len= *end - *start;
			return 0;
		}
	}

	CON_Out(console, "%sERR: Syntax error for range: format is addr,len or start_addr.end_addr%s", DT_color_red, DT_color_default);
	return -1;
}

/* ----------------------------------------------------------------------------
	display CPU memory pane. at the specified bank and address if given
	m [address]
*/
void cmd_dump_mem(int data, int argc, char* argv[]) {
	(void)data;
	dumpmode= DDUMP_RAM;
	if(argc == 1)
		return;
	unsigned int addr= cmd_eval_addr(argv[1]);
	if(addr == SYMBOL_NOT_FOUND) {
		CON_Out(console, "symbol \"%s\" not found", argv[1]);
		return;
	}
	currentBank= (addr & 0xFF0000) >> 16;
	currentData= addr & 0xFFFF;
}

/* ----------------------------------------------------------------------------
	display VIDEO memory pane. at the specified address if given
	v [address]
*/
void cmd_dump_videomem(int data, int argc, char* argv[]) {
	(void)data;
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
	(void)data;
	int idx= 2;
	int value;
	unsigned int addr= cmd_eval_addr(argv[1]);

	if(addr == SYMBOL_NOT_FOUND) {
		CON_Out(console, "symbol \"%s\" not found", argv[1]);
		return;
	}

	argc-= 2;
	switch(dumpmode) {
		case DDUMP_RAM:
		{
			int bank= addr >> 16;
			while(argc--) {
				value= (int)strtol(argv[idx++], NULL, 16);
				addr &= 0xFFFF;
				if(!isValidAddr(bank, addr)) {
					CON_Out(console, "%sERR: Invalid address %02X:%04X", DT_color_red, bank, addr, DT_color_default);
					return;
				}
				if (addr >= 0xA000) {
					if(addr < 0xC000)
						RAM[addr + (bank << 13)] = value;
					else {
						if(var_isROMdebugAllowed)
							ROM[(bank << 14) + addr - 0xc000]= value;
						else {
							CON_Out(console, "%sERR: Not allowed to write into ROM - use var ROM_DEBUG to change it%s", DT_color_red, DT_color_default);
							return;
						}

					}
				} else {
					RAM[addr] = value;
				}
				addr++;
			}
			break;
		}

		case DDUMP_VERA:
			while(argc--) {
				value= (int)strtol(argv[idx++], NULL, 16);
				addr &= 0x1FFFF;
				video_space_write(addr, value);
				addr++;
			}
			break;

	}
}

/* ----------------------------------------------------------------------------
	fill CPU/VIDEO memory depending on the displayed pane.
	f address value length [increment:1]
*/
void cmd_fill_memory(int data, int argc, char* argv[]) {
	(void)data;
	unsigned int addr= cmd_eval_addr(argv[1]);
	int value= (int)strtol(argv[2], NULL, 16);
	int length= (int)strtol(argv[3], NULL, 16);
	int incr= argc>4 ? (int)strtol(argv[4], NULL, 16) : 1;

	if(addr == SYMBOL_NOT_FOUND) {
		CON_Out(console, "symbol \"%s\" not found", argv[1]);
		return;
	}

	length= length > 0 ? length : 1;

	switch(dumpmode) {

		case DDUMP_RAM:
		{
			int bank= addr >> 16;
			while(length--) {
				addr &= 0xFFFF;
				if(!isValidAddr(bank, addr)) {
					CON_Out(console, "%sERR: Invalid address %02X:%04X", DT_color_red, bank, addr, DT_color_default);
					return;
				}
				if (addr >= 0xA000) {
					if(addr < 0xC000)
						RAM[addr + (bank << 13)] = value;
					else {
						if(var_isROMdebugAllowed)
							ROM[(bank << 14) + addr - 0xc000]= value;
						else{
							CON_Out(console, "%sERR: Not allowed to write into ROM - use var ROM_DEBUG to change it%s", DT_color_red, DT_color_default);
							return;
						}
					}
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
	search CPU/VIDEO memory in range (addr,len or start.end) for bytes
	sh address,length|start.end value0 value1 ...
*/
void cmd_search_memory(int data, int argc, char* argv[]) {
	(void)data;
	int bank, len, value, idx;
	unsigned int addr, addr_end;
	int memvalue= 0;
	bool partialfind= false;
	int foundAddr, foundCount= 0;

	if(-1 == cmd_get_range(argv[1], &addr, &addr_end, &len)) {
		return;
	}

	bank= (addr >> 16) & 0xFF;

	idx= 2;
	value= (int)strtol(argv[idx++], NULL, 16) & 0x00FF;

	while(len>0) {

		switch(dumpmode) {

			case DDUMP_RAM:
				addr &= 0xFFFF;
				if(!isValidAddr(bank, addr))
					continue;
				memvalue= real_read6502(addr, true, bank);
				break;

			case DDUMP_VERA:
				memvalue= video_space_read(addr);
				break;

		}

		if(value == memvalue) {
			if(!partialfind)
				foundAddr= addr;

			partialfind= true;

			if(idx == argc) {
				CON_Out(console, "%02d - %06X", foundCount, bank<<16 | foundAddr);
				partialfind= false;
				idx= 2;
				foundCount++;
			}

			value= (int)strtol(argv[idx++], NULL, 16) & 0x00FF;

		} else if(partialfind) {
			partialfind= false;
			idx= 2;
			value= (int)strtol(argv[idx++], NULL, 16) & 0x00FF;
			addr= foundAddr+1;
			continue;
		}

		addr++;
		len--;
	}

	CON_Out(console, "Total found %d", foundCount);

}

/* ----------------------------------------------------------------------------
	disasm from specified address
	d [bank]address
*/
void cmd_disasm(int data, int argc, char* argv[]) {
	(void)data;
	(void)argc;
	unsigned int addr= cmd_eval_addr(argv[1]);

	if(addr == SYMBOL_NOT_FOUND) {
		CON_Out(console, "symbol \"%s\" not found", argv[1]);
		return;
	}

	currentPCBank= addr >> 16;
	currentPC= addr & 0xFFFF;
}

/* ----------------------------------------------------------------------------
	set the ROM or RAM bank register
	b rom|ram bank
*/
void cmd_set_bank(int data, int argc, char* argv[]) {
	(void)data;
	(void)argc;
	char *type= argv[1];
	int value= (int)strtol(argv[2], NULL, 16) & 0x00FF;

	if(!strcmp(type, "rom")) {
		memory_set_rom_bank(value);
		return;
	}
	if(!strcmp(type, "ram")) {
		memory_set_ram_bank(value);
		return;
	}

	CON_Out(console, "%sERR: Bank is either ROM or RAM%s", DT_color_red, DT_color_default);

}

/* ----------------------------------------------------------------------------
	set the value of the specified register
	r A|X|Y|PC|SP|P|BKA|BKO|VA|VD0|VD1|VCT value
*/
void cmd_set_register(int data, int argc, char* argv[]) {
	(void)data;
	(void)argc;
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
	(void)argv;
	(void)argc;
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
	(void)data;
	char buffer[CON_CHARS_PER_LINE+1]= "\0";
	char *pBuffer= buffer;

	for(int idx= 0; cmd_table[idx].name; idx++) {
		if(argc == 2) {
			if(!strcmp(argv[1], cmd_table[idx].name)) {
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
	(void)data;
	(void)argc;
	(void)argv;
	if(breakpointsCount == 0) {
		CON_Out(console, "There is no current breakpoints");
		return;
	}
	char *labels[]= {
		"BP", "BPM", "BPR", "BPW"
	};
	for(int idx= 0; idx < breakpointsCount; idx++) {
		CON_Out(console, "%02d - %s %X", idx, labels[breakpoints[idx].type], breakpoints[idx].addr);
	}
}

/* ----------------------------------------------------------------------------
	add breakpoint
	bp address
*/
void cmd_bp_add(int data, int argc, char* argv[]) {
	(void)data;
	(void)argc;
	unsigned int addr= cmd_eval_addr(argv[1]);

	if(addr == SYMBOL_NOT_FOUND) {
		CON_Out(console, "symbol \"%s\" not found", argv[1]);
		return;
	}

	if(breakpointsCount >= DBG_MAX_BREAKPOINTS) {
		CON_Out(console, "You have reached the max number (%d) of breakpoints allowed", DBG_MAX_BREAKPOINTS);
		return;
	}

	int bank= addr >> 16;

	if(!isValidAddr(bank, addr)) {
		CON_Out(console, "%sERR: Invalid address%s", DT_color_red, DT_color_default);
		return ;
	}

	breakpointsCount++;
	breakpoints[breakpointsCount-1].type= BPT_PC;
	breakpoints[breakpointsCount-1].addr= addr;
}

/* ----------------------------------------------------------------------------
	add memory access breakpoint
	bpm address
*/
void cmd_bpm_add(int data, int argc, char* argv[]) {
	(void)argc;
	unsigned int addr= cmd_eval_addr(argv[1]);
	if(addr == SYMBOL_NOT_FOUND) {
		CON_Out(console, "symbol \"%s\" not found", argv[1]);
		return;
	}

	if(breakpointsCount >= DBG_MAX_BREAKPOINTS) {
		CON_Out(console, "You have reached the max number (%d) of breakpoints allowed", DBG_MAX_BREAKPOINTS);
		return;
	}

	int bank= addr >> 16;
	if(!isValidAddr(bank, addr & 0xFFFF)) {
		CON_Out(console, "%sERR: Invalid address%s", DT_color_red, DT_color_default);
		return;
	}

	breakpointsCount++;
	breakpoints[breakpointsCount-1].type= data;
	breakpoints[breakpointsCount-1].addr= addr;
}

/* ----------------------------------------------------------------------------
	clear a specific breakpoint or all
	bpc bp_number|*
*/
void cmd_bp_clear(int data, int argc, char* argv[]) {
	(void)data;
	(void)argc;
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
	sym address|symbol
*/
void cmd_sym(int data, int argc, char* argv[]) {
	(void)data;
	(void)argc;

	const char *addresses= symbol_lookup(argv[1]);
	if(addresses) {
		CON_Out(console, "%s: %s", argv[1], addresses);
		return;
	}

	TSymbolVolume *vol;
	int isFound= 0;
	unsigned int addr= cmd_eval_addr(argv[1]);
	if(addr != SYMBOL_NOT_FOUND) {
		for(int idx= 0; (vol= symbol_get_volume(idx)); idx++) {
			char *label= symbol_find_label(vol->bank, addr);
			if(label) {
				isFound= 1;
				CON_Out(console, "%s: %06X", label, (vol->bank << 16) | addr);
			}
		}
	}

	if(!isFound)
		CON_Out(console, "symbol \"%s\" not found", argv[1]);

}

/* ----------------------------------------------------------------------------
	clear the "symbol" or "*" all from the symbol table for specified bank
	symclear bank [symbol|*]
*/
void cmd_symclear(int data, int argc, char* argv[]) {
	(void)data;
	(void)argc;
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
	(void)data;

	int bank= -1;
	char *filename= NULL;
	int lineCount= 1;
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
	(void)data;
	(void)argc;

	symbol_dump(argv[1]);
}

/* ----------------------------------------------------------------------------
	display the code zone in full page
	code
*/
void cmd_code(int data, int argc, char* argv[]) {
	(void)data;
	(void)argc;
	(void)argv;

	DEBUGupdateLayout(layoutID != 1 ? 1 : 0);
}

/* ----------------------------------------------------------------------------
	display the data zone in full page
	data
*/
void cmd_data(int data, int argc, char* argv[]) {
	(void)data;
	(void)argc;
	(void)argv;

	DEBUGupdateLayout(layoutID != 2 ? 2 : 0);
}

/* ----------------------------------------------------------------------------
	Display loaded fonts or Set the debugger font or Load and Set the debugger font
	font [Name|ID [Path]]
*/
void cmd_font(int data, int argc, char* argv[]) {
	(void)data;

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
	display X16 emulator & debugger info
	info
*/
void cmd_info(int data, int argc, char* argv[]) {
	(void)data;
	(void)argc;
	(void)argv;

	CON_Out(console, "x16 emulator: Release %s (%s)", VER, VER_NAME);
	CON_Out(console, "x16 debugger: version %s", DBG_VER);
}

/* ----------------------------------------------------------------------------
	display CPU clock ticks counter
	ticks
*/
void cmd_ticks(int data, int argc, char* argv[]) {
	(void)data;
	(void)argc;
	(void)argv;
	CON_Out(console, "CPU clock ticks: %d", clockticks6502);
}

/* ----------------------------------------------------------------------------
	start/log/end timer
	time/timelog/timeend
*/
uint32_t timer;
void cmd_time(int data, int argc, char* argv[]) {
	(void)argc;

	uint32_t timeDiff= 0;

	switch(data) {
		case 0:
			timer= clockticks6502;
			CON_Out(console, "%s: timer started", argv[1]);
			return;

		case 1:
			timeDiff= clockticks6502 - timer;
			break;

		case 2:
			timeDiff= clockticks6502 - timer;
			timer= 0;
			break;
	}

	CON_Out(console, "%s: %d ticks [8MHz: %fs]", argv[1], timeDiff, ((float)timeDiff)/8/1000/1000);
}

/* ----------------------------------------------------------------------------
	load binary file in memory
	load file addr [[from:]len]
*/
void cmd_load(int data, int argc, char* argv[]) {
	(void)data;
	(void)argc;

	unsigned int addr= cmd_eval_addr(argv[2]);
	if(addr == SYMBOL_NOT_FOUND) {
		CON_Out(console, "%sERR: symbol \"%s\" not found%s", DT_color_red, argv[2], DT_color_default);
		return;
	}

	int bank= addr >> 16;
	addr &= 0xFFFF;

	if(!isValidAddr(bank, addr)) {
		CON_Out(console, "%sERR: Invalid address%s", DT_color_red, DT_color_default);
		return;
	}

	Sint64 len= -1;
	Sint64 from= 0;

	if(argc>3) {
		char *lenStr= strchr(argv[3], ':');
		if(lenStr != NULL) {
			from= strtol(argv[3], NULL, 16);
			lenStr++;
		} else {
			lenStr= argv[3];
		}
		len= strtol(lenStr, NULL, 16);
		len= len ? len : -1;
	}

	SDL_RWops *fp= SDL_RWFromFile(argv[1], "rb");
	if(fp == NULL) {
		CON_Out(console, "%sERR: Can't open file%s - %s", DT_color_red, argv[1], SDL_GetError(), DT_color_default);
		return;
	}
	Sint64 filesize= SDL_RWsize(fp);
	if(len == -1) {
		len= filesize - from;
	}

	uint8_t *MEMPtr;
	int bankEnd= bank;
	int offset;
	unsigned int addrEnd= addr + len - 1;
	int err= 0;
	// BANKED ROM - check if allowed / overflow
	if(addr >= 0xC000) {
		offset= (bank << 14) + addr - 0xc000;
		if(!var_isROMdebugAllowed) {
			CON_Out(console, "%sERR: Not allowed to load into ROM - use var ROM_DEBUG to change it%s", DT_color_red, DT_color_default);
			err= 1;
		}
		else if((bank >= NUM_ROM_BANKS) || (offset+len > ROM_SIZE)) {
			CON_Out(console, "%sERR: Banked ROM overflow (%d bytes in %d banks) %s", DT_color_red, ROM_SIZE, NUM_ROM_BANKS, DT_color_default);
			err= 1;
		}
		else {
			MEMPtr= &ROM[offset];
		}
	}
	// BANKED RAM - check if overflow
	else if(addr >= 0xA000) {
		offset= (bank << 13) + addr;
		if((bank >= num_ram_banks) || (offset+len > RAM_SIZE)) {
			CON_Out(console, "%sERR: Banked RAM overflow (%d bytes in %d banks) %s", DT_color_red, num_ram_banks*8192, num_ram_banks, DT_color_default);
			err= 1;
		} else {
			MEMPtr= &RAM[offset];
			Sint64 byteCount= addrEnd - 0xA000;
			bankEnd= bank + byteCount / 8192;
			addrEnd= 0xA000 + byteCount % 8192;
		}
	}
	// IO space - NOT ALLOWED
	else if( (addr >= 0x9F00) || ((addrEnd >= 0x9F00) && (addrEnd <= 0xA000))) {
		CON_Out(console, "%sERR: Unable to load into IO space $9F00.$9FFF%s", DT_color_red, DT_color_default);
		err= 1;
	}
	else {
		MEMPtr= &RAM[addr];
	}

	if(err) {
		SDL_RWclose(fp);
		return;
	}

	SDL_RWseek(fp, from, RW_SEEK_SET);
	SDL_RWread(fp, MEMPtr, len, 1);
	SDL_RWclose(fp);

	CON_Out(console, "loaded file to %02X:%04X.%02X:%04X", bank, addr, bankEnd, addrEnd);
}

/*

*/
void cmd_var_printvar(const dictionary * d, const char *key, const char *entry) {
	(void)d;
	(void)key;
	const char *info;
	int value= var_get(entry, &info);
	CON_Out(console, "    %s = %d ; %s", entry, value, info);
}

/* ----------------------------------------------------------------------------
	list settings vars or set or get value for a settings var
	var [name [value]]
*/
void cmd_var(int data, int argc, char* argv[]) {
	(void)data;

	switch(argc) {
		case 1:
		{
			var_get_list(cmd_var_printvar);
			return;
		}

		case 2:
		{
			if(var_exists(argv[1]))  {
				cmd_var_printvar(NULL, NULL, argv[1]);
			} else {
				CON_Out(console, "%sERR: var not found %s", DT_color_red, DT_color_default);
			}
			return;
		}

		case 3:
		{
			if(var_exists(argv[1]))  {
				var_set(argv[1], strtol(argv[2], NULL, 0));
				cmd_var_printvar(NULL, NULL, argv[1]);
			} else {
				CON_Out(console, "%sERR: var not found %s", DT_color_red, DT_color_default);
			}
			return;
		}
	}


}
