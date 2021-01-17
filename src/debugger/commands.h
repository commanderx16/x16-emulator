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

extern command_t cmd_table[];

void cmd_dump_mem(int data, int argc, char* argv[]);
void cmd_dump_videomem(int data, int argc, char* argv[]);
void cmd_edit_mem(int data, int argc, char* argv[]);
void cmd_fill_memory(int data, int argc, char* argv[]);
void cmd_search_memory(int data, int argc, char* argv[]);
void cmd_disasm(int data, int argc, char* argv[]);
void cmd_set_bank(int data, int argc, char* argv[]);
void cmd_set_register(int data, int argc, char* argv[]);
void cmd_set_status(int data, int argc, char* argv[]);

void cmd_help(int data, int argc, char* argv[]);

void cmd_bp_list(int data, int argc, char* argv[]);
void cmd_bp_add(int data, int argc, char* argv[]);
void cmd_bpm_add(int data, int argc, char* argv[]);
void cmd_bp_clear(int data, int argc, char* argv[]);

void cmd_sym(int data, int argc, char* argv[]);
void cmd_symclear(int data, int argc, char* argv[]);
void cmd_symload(int data, int argc, char* argv[]);
void cmd_symsave(int data, int argc, char* argv[]);

void cmd_code(int data, int argc, char* argv[]);
void cmd_data(int data, int argc, char* argv[]);
void cmd_font(int data, int argc, char* argv[]);

void cmd_romdebug(int data, int argc, char* argv[]);
void cmd_info(int data, int argc, char* argv[]);

void cmd_load(int data, int argc, char* argv[]);


#endif
