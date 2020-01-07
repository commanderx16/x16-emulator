// *******************************************************************************************
// *******************************************************************************************
//
//		File:		debugger.c
//		Date:		5th September 2019
//		Purpose:	Debugger code
//		Author:		Paul Robson (paul@robson.org.uk)
//
// *******************************************************************************************
// *******************************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <SDL.h>

#include "glue.h"
#include "disasm.h"
#include "memory.h"
#include "video.h"
#include "cpu/fake6502.h"
#include "debugger.h"
#include "rendertext.h"

static void DEBUGHandleKeyEvent(SDL_Keycode key,int isShift);

static void DEBUGNumber(int x,int y,int n,int w, SDL_Color colour);
static void DEBUGAddress(int x, int y, int bank, int addr, SDL_Color colour);

static void DEBUGRenderData(int y,int data);
static int DEBUGRenderRegisters(void);
static void DEBUGRenderCode(int lines,int initialPC);
static void DEBUGRenderStack(int bytesCount);
static void DEBUGRenderCmdLine();
static bool DEBUGBuildCmdLine(SDL_Keycode key);
static void DEBUGExecCmd();

// *******************************************************************************************
//
//		This is the minimum-interference flag. It's designed so that when
//		its non-zero DEBUGRenderDisplay() is called.
//
//			if (showDebugOnRender != 0) {
//				DEBUGRenderDisplay(SCREEN_WIDTH,SCREEN_HEIGHT,renderer);
//				SDL_RenderPresent(renderer);
//				return true;
//			}
//
//		before the SDL_RenderPresent call in video_update() in video.c
//
//		This controls what is happening. It is at the top of the main loop in main.c
//
//			if (isDebuggerEnabled != 0) {
//				int dbgCmd = DEBUGGetCurrentStatus();
//				if (dbgCmd > 0) continue;
//				if (dbgCmd < 0) break;
//			}
//
//		Both video.c and main.c require debugger.h to be included.
//
//		isDebuggerEnabled should be a flag set as a command line switch - without it
//		it will run unchanged. It should not be necessary to test the render code
//		because showDebugOnRender is statically initialised to zero and will only
//		change if DEBUGGetCurrentStatus() is called.
//
// *******************************************************************************************

//
//				0-9A-F sets the program address, with shift sets the data address.
//
#define DBGKEY_HOME 	SDLK_F1 								// F1 is "Goto PC"
#define DBGKEY_RESET 	SDLK_F2 								// F2 resets the 6502
#define DBGKEY_RUN 		SDLK_F5 								// F5 is run.
#define DBGKEY_SETBRK 	SDLK_F9									// F9 sets breakpoint
#define DBGKEY_STEP 	SDLK_F11 								// F11 is step into.
#define DBGKEY_STEPOVER	SDLK_F10 								// F10 is step over.
#define DBGKEY_PAGE_NEXT	SDLK_KP_PLUS
#define DBGKEY_PAGE_PREV	SDLK_KP_MINUS

#define DBGSCANKEY_BRK 	SDL_SCANCODE_F12 						// F12 is break into running code.
#define DBGSCANKEY_SHOW	SDL_SCANCODE_TAB 						// Show screen key.
																// *** MUST BE SCAN CODES ***

enum DBG_CMD { CMD_DUMP_MEM='m', CMD_DISASM='d', CMD_SET_BANK='b', CMD_SET_REGISTER='r' };

// RGB colours
const SDL_Color col_bkgnd= {0, 0, 0, 255};
const SDL_Color col_label= {0, 255, 0, 255};
const SDL_Color col_data= {0, 255, 255, 255};
const SDL_Color col_highlight= {255, 255, 0, 255};
const SDL_Color col_cmdLine= {255, 255, 255, 255};

int showDebugOnRender = 0;										// Used to trigger rendering in video.c
int showFullDisplay = 0; 										// If non-zero show the whole thing.
int currentPC = -1;												// Current PC value.
int currentData = 0;											// Current data display address.
int currentPCBank = -1;
int currentBank = -1;
int currentMode = DMODE_RUN;									// Start running.
int breakPoint = -1; 											// User Break
int stepBreakPoint = -1;										// Single step break.

char cmdLine[64]= "";											// command line buffer
int currentPosInLine= 0;										// cursor position in the buffer (NOT USED _YET_)
int currentLineLen= 0;											// command line buffer length

//
//		This flag controls
//

SDL_Renderer *dbgRenderer; 										// Renderer passed in.

// *******************************************************************************************
//
//			This is used to determine who is in control. If it returns zero then
//			everything runs normally till the next call.
//			If it returns +ve, then it will update the video, and loop round.
//			If it returns -ve, then exit.
//
// *******************************************************************************************

int  DEBUGGetCurrentStatus(void) {

	SDL_Event event;
	if (currentPC < 0) currentPC = pc;							// Initialise current PC displayed.

	if (currentMode == DMODE_STEP) {							// Single step before
		currentPC = pc;											// Update current PC
		currentMode = DMODE_STOP;								// So now stop, as we've done it.
	}

	if (pc == breakPoint || pc == stepBreakPoint) {				// Hit a breakpoint.
		currentPC = pc;											// Update current PC
		currentMode = DMODE_STOP;								// So now stop, as we've done it.
		stepBreakPoint = -1;									// Clear step breakpoint.
	}

	if (SDL_GetKeyboardState(NULL)[DBGSCANKEY_BRK]) {			// Stop on break pressed.
		currentMode = DMODE_STOP;
		currentPC = pc; 										// Set the PC to what it is.
	}

	if(currentPCBank<0 && currentPC >= 0xA000) {
		currentPCBank= currentPC < 0xC000 ? memory_get_ram_bank() : memory_get_rom_bank();
	}

	if (currentMode != DMODE_RUN) {								// Not running, we own the keyboard.
		showFullDisplay = 										// Check showing screen.
					SDL_GetKeyboardState(NULL)[DBGSCANKEY_SHOW];
		while (SDL_PollEvent(&event)) { 						// We now poll events here.
			switch(event.type) {
				case SDL_QUIT:									// Time for exit
					return -1;

				case SDL_KEYDOWN:								// Handle key presses.
					DEBUGHandleKeyEvent(event.key.keysym.sym,
										SDL_GetModState() & (KMOD_LSHIFT|KMOD_RSHIFT));
					break;

			}
		}
	}

	showDebugOnRender = (currentMode != DMODE_RUN);				// Do we draw it - only in RUN mode.
	if (currentMode == DMODE_STOP) { 							// We're in charge.
		video_update();
		return 1;
	}

	return 0;													// Run wild, run free.
}

// *******************************************************************************************
//
//								Setup fonts and co
//
// *******************************************************************************************
void DEBUGInitUI(SDL_Renderer *pRenderer) {
		DEBUGInitChars(pRenderer);
		dbgRenderer = pRenderer;				// Save renderer.
}

// *******************************************************************************************
//
//								Setup fonts and co
//
// *******************************************************************************************
void DEBUGFreeUI() {
}

// *******************************************************************************************
//
//								Set a new breakpoint address. -1 to disable.
//
// *******************************************************************************************

void DEBUGSetBreakPoint(int newBreakPoint) {
	breakPoint = newBreakPoint;
}

// *******************************************************************************************
//
//								Break into debugger from code.
//
// *******************************************************************************************

void DEBUGBreakToDebugger(void) {
	currentMode = DMODE_STOP;
	currentPC = pc;
}

// *******************************************************************************************
//
//									Handle keyboard state.
//
// *******************************************************************************************

static void DEBUGHandleKeyEvent(SDL_Keycode key,int isShift) {
	int opcode;

	switch(key) {

		case DBGKEY_STEP:									// Single step (F11 by default)
			currentMode = DMODE_STEP; 						// Runs once, then switches back.
			break;

		case DBGKEY_STEPOVER:								// Step over (F10 by default)
			opcode = real_read6502(pc, false, 0);							// What opcode is it ?
			if (opcode == 0x20) { 							// Is it JSR ?
				stepBreakPoint = pc + 3;					// Then break 3 on.
				currentMode = DMODE_RUN;					// And run.
			} else {
				currentMode = DMODE_STEP;					// Otherwise single step.
			}
			break;

		case DBGKEY_RUN:									// F5 Runs until Break.
			currentMode = DMODE_RUN;
			break;

		case DBGKEY_SETBRK:									// F9 Set breakpoint to displayed.
			breakPoint = currentPC;
			break;

		case DBGKEY_HOME:									// F1 sets the display PC to the actual one.
			currentPC = pc;
			currentPCBank= currentPC < 0xC000 ? memory_get_ram_bank() : memory_get_rom_bank();
			break;

		case DBGKEY_RESET:									// F2 reset the 6502
			reset6502();
			currentPC = pc;
			currentPCBank= -1;
			break;

		case DBGKEY_PAGE_NEXT:
			currentBank += 1;
			break;

		case DBGKEY_PAGE_PREV:
			currentBank -= 1;
			break;

		case SDLK_PAGEDOWN:
			currentData= (currentData + 0xD0) & 0xFFFF;
			break;

		case SDLK_PAGEUP:
			currentData= (currentData - 0xD0) & 0xFFFF;
			break;

		default:
			if(DEBUGBuildCmdLine(key)) {
				// printf("cmd line: %s\n", cmdLine);
				DEBUGExecCmd();
			}
			break;
	}

}

char kNUM_KEYPAD_CHARS[10] = {'1','2','3','4','5','6','7','8','9','0'};

static bool DEBUGBuildCmdLine(SDL_Keycode key) {
	// right now, let's have a rudimentary input: only backspace to delete last char
	// later, I want a real input line with delete, backspace, left and right cursor
	// devs like their comfort ;)
	if(currentLineLen <= sizeof(cmdLine)) {
		if(
			(key >= SDLK_SPACE && key <= SDLK_AT)
			||
			(key >= SDLK_LEFTBRACKET && key <= SDLK_z)
			||
			(key >= SDLK_KP_1 && key <= SDLK_KP_0)
			) {
			cmdLine[currentPosInLine++]= key>=SDLK_KP_1 ? kNUM_KEYPAD_CHARS[key-SDLK_KP_1] : key;
			if(currentPosInLine > currentLineLen) {
				currentLineLen++;
			}
		} else if(key == SDLK_BACKSPACE) {
			currentPosInLine--;
			if(currentPosInLine<0) {
				currentPosInLine= 0;
			}
			currentLineLen--;
			if(currentLineLen<0) {
				currentLineLen= 0;
			}
		}
		cmdLine[currentLineLen]= 0;
	}
	return (key == SDLK_RETURN) || (key == SDLK_KP_ENTER);
}

static void DEBUGExecCmd() {
	int number, addr;
	char reg[10];
	char cmd;
	char *line= ltrim(cmdLine);

	cmd= *line;
	if(*line) {
		line++;
	}
	// printf("cmd:%c line: '%s'\n", cmd, line);

	switch (cmd) {
		case CMD_DUMP_MEM:
			sscanf(line, "%x", &number);
			addr= number & 0xFFFF;
			// Banked Memory, RAM then ROM
			if(addr >= 0xA000) {
				currentBank= (number & 0xFF0000) >> 16;
			}
			currentData= addr;
			break;

		case CMD_DISASM:
			sscanf(line, "%x", &number);
			addr= number & 0xFFFF;
			// Banked Memory, RAM then ROM
			if(addr >= 0xA000) {
				currentPCBank= (number & 0xFF0000) >> 16;
			}
			currentPC= addr;
			break;

		case CMD_SET_BANK:
			sscanf(line, "%s %d", reg, &number);

			if(!strcmp(reg, "rom")) {
				memory_set_rom_bank(number & 0x00FF);
			}
			if(!strcmp(reg, "ram")) {
				memory_set_ram_bank(number & 0x00FF);
			}
			break;

		case CMD_SET_REGISTER:
			sscanf(line, "%s %x", reg, &number);

			if(!strcmp(reg, "pc")) {
				pc= number & 0xFFFF;
			}
			if(!strcmp(reg, "a")) {
				a= number & 0x00FF;
			}
			if(!strcmp(reg, "x")) {
				x= number & 0x00FF;
			}
			if(!strcmp(reg, "y")) {
				y= number & 0x00FF;
			}
			if(!strcmp(reg, "sp")) {
				sp= number & 0x00FF;
			}
			break;

		default:
			break;
	}

	currentPosInLine= currentLineLen= *cmdLine= 0;
}

// *******************************************************************************************
//
//							Render the emulator debugger display.
//
// *******************************************************************************************

void DEBUGRenderDisplay(int width, int height) {
	if (showFullDisplay) return;								// Not rendering debug.

	SDL_Rect rc;
	rc.w = DBG_WIDTH * 6 * CHAR_SCALE;							// Erase background, set up rect
	rc.h = height;
	xPos = width-rc.w;yPos = 0; 								// Position screen
	rc.x = xPos;rc.y = yPos; 									// Set rectangle and black out.
	SDL_SetRenderDrawColor(dbgRenderer,0,0,0,SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(dbgRenderer,&rc);

	DEBUGRenderRegisters();							// Draw register name and values.
	DEBUGRenderCode(20, currentPC);							// Render 6502 disassembly.
	DEBUGRenderData(21, currentData);
	DEBUGRenderStack(20);

	DEBUGRenderCmdLine(xPos, rc.w, height);
}

// *******************************************************************************************
//
//									 Render command Line
//
// *******************************************************************************************

static void DEBUGRenderCmdLine(int x, int width, int height) {
	char buffer[sizeof(cmdLine)+1];

	SDL_SetRenderDrawColor(dbgRenderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
	SDL_RenderDrawLine(dbgRenderer, x, height-12, x+width, height-12);

	sprintf(buffer, ">%s", cmdLine);
	DEBUGString(dbgRenderer, 0, DBG_HEIGHT-1, buffer, col_cmdLine);
}
// *******************************************************************************************
//
//									 Render Data Area
//
// *******************************************************************************************

static void DEBUGRenderData(int y,int data) {
	while (y < DBG_HEIGHT-2) {									// To bottom of screen
		DEBUGAddress(DBG_MEMX, y, (uint8_t)currentBank, data & 0xFFFF, col_label);	// Show label.

		for (int i = 0;i < 8;i++) {
			int byte= real_read6502((data+i) & 0xFFFF, true, currentBank);
			DEBUGNumber(DBG_MEMX+8+i*3,y,byte,2, col_data);
			DEBUGWrite(dbgRenderer, DBG_MEMX+33+i,y,byte, col_data);
		}
		y++;
		data += 8;
	}
}

// *******************************************************************************************
//
//									 Render Disassembly
//
// *******************************************************************************************

static void DEBUGRenderCode(int lines, int initialPC) {
	char buffer[32];
	for (int y = 0; y < lines; y++) { 							// Each line

		DEBUGAddress(DBG_ASMX, y, currentPCBank, initialPC, col_label);

		int size = disasm(initialPC, RAM, buffer, sizeof(buffer), true, currentPCBank);	// Disassemble code
		// Output assembly highlighting PC
		DEBUGString(dbgRenderer, DBG_ASMX+8, y, buffer, initialPC == pc ? col_highlight : col_data);
		initialPC += size;										// Forward to next
	}
}

// *******************************************************************************************
//
//									Render Register Display
//
// *******************************************************************************************

static char *labels[] = { "NV-BDIZC","","","A","X","Y","","BKA","BKO", "PC","SP","","BRK","", "VA","VD0","VD1","VCT", NULL };


static int DEBUGRenderRegisters(void) {
	int n = 0,yc = 0;
	while (labels[n] != NULL) {									// Labels
		DEBUGString(dbgRenderer, DBG_LBLX,n,labels[n], col_label);n++;
	}
	yc++;
	DEBUGNumber(DBG_LBLX, yc, (status >> 7) & 1, 1, col_data);
	DEBUGNumber(DBG_LBLX+1, yc, (status >> 6) & 1, 1, col_data);
	DEBUGNumber(DBG_LBLX+3, yc, (status >> 4) & 1, 1, col_data);
	DEBUGNumber(DBG_LBLX+4, yc, (status >> 3) & 1, 1, col_data);
	DEBUGNumber(DBG_LBLX+5, yc, (status >> 2) & 1, 1, col_data);
	DEBUGNumber(DBG_LBLX+6, yc, (status >> 1) & 1, 1, col_data);
	DEBUGNumber(DBG_LBLX+7, yc, (status >> 0) & 1, 1, col_data);
	yc+= 2;

	DEBUGNumber(DBG_DATX, yc++, a, 2, col_data);
	DEBUGNumber(DBG_DATX, yc++, x, 2, col_data);
	DEBUGNumber(DBG_DATX, yc++, y, 2, col_data);
	yc++;

	DEBUGNumber(DBG_DATX, yc++, memory_get_ram_bank(), 2, col_data);
	DEBUGNumber(DBG_DATX, yc++, memory_get_rom_bank(), 2, col_data);
	DEBUGNumber(DBG_DATX, yc++, pc, 4, col_data);
	DEBUGNumber(DBG_DATX, yc++, sp|0x100, 4, col_data);
	yc++;

	DEBUGNumber(DBG_DATX, yc++, breakPoint & 0xFFFF, 4, col_data);
	yc++;

	DEBUGNumber(DBG_DATX, yc++, video_read(0, true) | (video_read(1, true)<<8) | (video_read(2, true)<<16), 2, col_data);
	DEBUGNumber(DBG_DATX, yc++, video_read(3, true), 2, col_data);
	DEBUGNumber(DBG_DATX, yc++, video_read(4, true), 2, col_data);
	DEBUGNumber(DBG_DATX, yc++, video_read(5, true), 2, col_data);

	return n; 													// Number of code display lines
}

// *******************************************************************************************
//
//									Render Top of Stack
//
// *******************************************************************************************

static void DEBUGRenderStack(int bytesCount) {
	int data= (sp+1) | 0x100;
	int y= 0;
	while (y < bytesCount) {
		DEBUGNumber(DBG_STCK,y,data & 0xFFFF,4, col_label);
		int byte = real_read6502((data++) & 0xFFFF, false, 0);
		DEBUGNumber(DBG_STCK+5,y,byte,2, col_data);
		DEBUGWrite(dbgRenderer, DBG_STCK+9,y,byte, col_data);
		y++;
		data= (data & 0xFF) | 0x100;
	}
}

// *******************************************************************************************
//
//									Write Hex Constant
//
// *******************************************************************************************

static void DEBUGNumber(int x, int y, int n, int w, SDL_Color colour) {
	char fmtString[8],buffer[16];
	snprintf(fmtString, sizeof(fmtString), "%%0%dX", w);
	snprintf(buffer, sizeof(buffer), fmtString, n);
	DEBUGString(dbgRenderer, x, y, buffer, colour);
}

// *******************************************************************************************
//
//									Write Bank:Address
//
// *******************************************************************************************
static void DEBUGAddress(int x, int y, int bank, int addr, SDL_Color colour) {
	char buffer[4];

	if(addr >= 0xA000) {
		snprintf(buffer, sizeof(buffer), "%.2X:", bank);
	} else {
		strcpy(buffer, "--:");
	}

	DEBUGString(dbgRenderer, x, y, buffer, colour);

	DEBUGNumber(x+3, y, addr, 4, colour);

}
