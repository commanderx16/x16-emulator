#ifndef _COMMANDS_H
#define _COMMANDS_H

#define DBG_MAX_BREAKPOINTS 10

typedef struct {
	char* name;
	void (*fn)(int data, int argc, char* argv[]);
	int minargc;
	int data;
	char* help;
} command_t;

enum register_code {
		REG_PC= 1,
		REG_A, REG_X, REG_Y,
		REG_SP,
		REG_P,
		REG_BKA, REG_BKO,
		REG_VA, REG_VD0, REG_VD1, REG_VCT,
		REG_R0, REG_R1, REG_R2, REG_R3, REG_R4, REG_R5, REG_R6, REG_R7,
		REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15,
		REG_x16, REG_x17, REG_x18, REG_x19,
		REG_BP0, REG_BP1, REG_BP2, REG_BP3, REG_BP4,
		REG_BP5, REG_BP6, REG_BP7, REG_BP8, REG_BP9
};

extern command_t cmd_table[];

void cmd_dump_mem(int data, int argc, char* argv[]);
void cmd_dump_videomem(int data, int argc, char* argv[]);
void cmd_edit_mem(int data, int argc, char* argv[]);
void cmd_fill_memory(int data, int argc, char* argv[]);
void cmd_disasm(int data, int argc, char* argv[]);
void cmd_set_bank(int data, int argc, char* argv[]);
void cmd_set_register(int data, int argc, char* argv[]);
void cmd_set_status(int data, int argc, char* argv[]);

void cmd_help(int data, int argc, char* argv[]);

void cmd_bp_list(int data, int argc, char* argv[]);
void cmd_bp_add(int data, int argc, char* argv[]);
void cmd_bp_clear(int data, int argc, char* argv[]);

void cmd_sym(int data, int argc, char* argv[]);
void cmd_symclear(int data, int argc, char* argv[]);
void cmd_symload(int data, int argc, char* argv[]);
void cmd_symsave(int data, int argc, char* argv[]);

void cmd_code(int data, int argc, char* argv[]);

void cmd_romdebug(int data, int argc, char* argv[]);


#endif
