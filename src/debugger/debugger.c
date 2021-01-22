
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
#include <string.h>
#include <inttypes.h>
#include <SDL.h>
#include "../glue.h"
#include "disasm.h"
#include "../memory.h"
#include "../video.h"
#include "../cpu/fake6502.h"
#include "debugger.h"
#include "../console/SDL_console.h"
#include "../console/DT_drawtext.h"
#include "../console/split.h"
#include "commands.h"
#include "symbols.h"
#include "breakpoints.h"

static int DEBUGHandleKeyEvent(SDL_Event *event);

static void DEBUGRenderCode(int col, int row, int lineCount);
static void DEBUGRenderData(int col, int row, int lineCount);
static void DEBUGRenderVRAM(int col, int row, int lineCount);
static void DEBUGRenderRegisters(int col, int row, TRegister_kind regKing);
static void DEBUGRenderStack(int bytesCount);

static void DEBUG_Command_Handler(ConsoleInformation *console, char* command);

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

#ifdef __APPLE__
#define CMD_KEY KMOD_GUI
#else
#define CMD_KEY KMOD_CTRL
#endif

//
//				0-9A-F sets the program address, with shift sets the data address.
//
/*
#define DBGKEY_HOME 	SDLK_F1 								// F1 is "Goto PC"
#define DBGKEY_RESET 	SDLK_F2 								// F2 resets the 6502
int DBGKEY_RUN= 		SDLK_F5; 								// F5 is run.
#define DBGKEY_SETBRK 	SDLK_F9									// F9 sets breakpoint
#define DBGKEY_STEP 	SDLK_F11 								// F11 is step into.
#define DBGKEY_STEPOVER	SDLK_F10 								// F10 is step over.
#define DBGKEY_PAGE_NEXT	SDLK_KP_PLUS
#define DBGKEY_PAGE_PREV	SDLK_KP_MINUS
*/

enum {
		DBGKEY_HOME= 1,
		DBGKEY_RESET,
		DBGKEY_RUN,
		DBGKEY_SETBRK,
		DBGKEY_STEP,
		DBGKEY_STEPOVER,
		DBGKEY_PAGE_NEXT,
		DBGKEY_PAGE_PREV,
		DBGKEY_PASTE,
};

typedef struct {
	char *name;
	int binding;
	SDL_Keycode key;
	SDL_Keymod keyMod;
} KeyBinding;

KeyBinding keyBindings[]= {
	{"DBG_STEP", DBGKEY_STEP, SDLK_F11, 0xFFFF},
	{"DBG_STEPOVER", DBGKEY_STEPOVER, SDLK_F10, 0xFFFF},
	{"DBG_RUN", DBGKEY_RUN, SDLK_F5, 0xFFFF},
	{"DBG_SETBRK", DBGKEY_SETBRK, SDLK_F9, 0xFFFF},
	{"DBG_HOME", DBGKEY_HOME, SDLK_F1, 0xFFFF},
	{"DBG_RESET", DBGKEY_RESET, SDLK_F2, 0xFFFF},
	{"DBG_PASTE", DBGKEY_PASTE, SDLK_v, CMD_KEY},
	{NULL, 0, 0, 0},
};

#define DBGSCANKEY_BRK 	SDL_SCANCODE_F12 						// F12 is break into running code.
																// *** MUST BE SCAN CODES ***

#define DBGMAX_ZERO_PAGE_REGISTERS 20

int showDebugOnRender = 0;										// Used to trigger rendering in video.c
int currentPC = -1;												// Current PC value.
int currentData = 0;											// Current data display address.
int currentPCBank = -1;
int currentBank = 0;
int currentMode = DMODE_RUN;									// Start running.
int breakPoint = -1; 											// User Break
int stepBreakPoint = -1;										// Single step break.
int dumpmode          = DDUMP_RAM;
int showFullConsole = 0;
extern int var_BPonBRK;

TBreakpoint breakpoints[DBG_MAX_BREAKPOINTS];
int breakpointsCount= 0;

int isWindowVisible = 0;
int disasmLine1Size= 0;
int offsetSP= 0;
int wannaShowMouseCoord= 0;

//
// layout related stuff
//

// const int DBG_LAYOUT_CODE_WIDTH= 8 + 9 + 14 + 3+1+13+3;
const int DBG_LAYOUT_REG_WIDTH= 4 + 3 + 4 + 3;
const int DBG_LAYOUT_ZP_REG_WIDTH= 4 + 4;
const int DBG_LAYOUT_STCK_WIDTH= 4 + 1 + 2 + 1 + 1;
const int DBG_LAYOUT_DATA_WIDTH= 8 + 16 * 3 + 16;

// COLOURS

SDL_Color col_label= {0, 255, 0, 255};
SDL_Color col_data= {0, 255, 255, 255};
SDL_Color col_highlight= {255, 255, 0, 255};
SDL_Color col_dis_fnlabel= {156, 220, 254, 255};
SDL_Color col_dis_sublabel= {147, 206, 137, 255};

const int col_vram_len= 4;
/*
	0 : tilemap
	1 : tiledata
	2 : special
	3 : other
*/
SDL_Color col_vram[] = {
	{0, 255, 255, 255},
	{0, 255, 0, 255},
	{255, 92, 92, 255},
	{128, 128, 128, 255}
};

SDL_Color col_dbg_bkgnd = {0, 0, 0, 255};
SDL_Color col_con_bkgnd = {0, 0, 0, 255};
SDL_Color col_dbg_focus = {255, 255, 255, 0};

// main variables

int win_height = 800;
int win_width = 640;
int con_height = 50;
int layoutID= 0;
TLayout layout;
int dbgFontID= 0;

// UI zones

SDL_Rect smallDataZoneRect= { 0, 288, 525, 455 };
SDL_Rect largeDataZoneRect= { 0, 0, 525, 750 };
SDL_Rect smallCodeZoneRect= { 5, 0, 370, 285 };
SDL_Rect largeCodeZoneRect= { 5, 0, 370, 750 };
SDL_Rect stackZoneRect= { 575, 0, 65, 285 };
SDL_Rect zpRegZoneRect= { 575, 288, 65, 455 };
SDL_Rect emptyZoneRect= { 0, 0, 0, 0 };

enum { MZ_NONE, MZ_CODE, MZ_DATA, MZ_STACK, MZ_ZPREG, mouseZonesCount};
int mouseZone= MZ_NONE;
TMouseZone mouseZones[]= {
	{MZT_NONE, NULL},
	{MZT_HIGHLIGHT, &smallCodeZoneRect},
	{MZT_HIGHLIGHT, &smallDataZoneRect},
	{MZT_HIGHLIGHT, &stackZoneRect},
	{MZT_NONE, &zpRegZoneRect}
};

//
// main objects
//

SDL_Renderer *dbgRenderer; 										// Renderer passed in.
static SDL_Window* dbgWindow;
static uint32_t dbgWindowID;
ConsoleInformation *console;

// *******************************************************************************************
//
//									test if there's a BP on addr
//
// *******************************************************************************************
bool DEBUGisOnBreakpoint(int addr, TBreakpointType type) {
	int addr24= memory_get_bank(addr) << 16 | addr;
	for(int idx= 0; idx<breakpointsCount; idx++) {
		if((breakpoints[idx].type & type) && breakpoints[idx].addr == addr24)
			return true;
	}
	return false;
}

// *******************************************************************************************
//
//									Place the dbg in STOP mode
//
// *******************************************************************************************
void DEBUGstop() {
	currentPC = pc;
	currentPCBank= -1;
	currentMode = DMODE_STOP;
}

// *******************************************************************************************
//
//									Write String at postion col,line
//
// *******************************************************************************************
void DEBUGPrintString(SDL_Renderer *renderer, int col, int row, char *s, SDL_Color colour) {
	int h= DT_FontHeight(dbgFontID) + 1;
	int w= DT_FontWidth(dbgFontID);
	DT_DrawText2(renderer, s, dbgFontID, col*w, row*h, colour);
}

void DEBUGPrintChar(SDL_Renderer *renderer, int col, int row, int ch, SDL_Color colour) {
	char buffer[2];
	sprintf(buffer, "%c", ch);
	DEBUGPrintString(renderer, col, row, buffer, colour);
}

// *******************************************************************************************
//
//									Write Hex/Bin Constant
//
// *******************************************************************************************
const char *bit_rep[16] = {
    [ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
    [ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
    [ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
    [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
};

static void DEBUGPrintNumber(int col, int row, int n, int w, SDL_Color colour) {
	char fmtString[8],buffer[16];
	if(w<0) {
		snprintf(buffer, sizeof(buffer), "%s%s", bit_rep[n >> 4], bit_rep[n & 0x0F]);
	} else {
		snprintf(fmtString, sizeof(fmtString), "%%0%dX", w);
		snprintf(buffer, sizeof(buffer), fmtString, n);
	}
	DEBUGPrintString(dbgRenderer, col, row, buffer, colour);
}

// *******************************************************************************************
//
//									Write Bank:Address
//
// *******************************************************************************************
static void DEBUGPrintAddress(int col, int row, int bank, int addr, SDL_Color colour) {
	char buffer[4];

	if(addr >= 0xA000) {
		snprintf(buffer, sizeof(buffer), "%.2X:", bank);
	} else {
		strcpy(buffer, "--:");
	}

	DEBUGPrintString(dbgRenderer, col, row, buffer, colour);

	DEBUGPrintNumber(col+3, row, addr, 4, colour);

}

static void
DEBUGPrintVAddress(int col, int row, int addr, SDL_Color colour)
{
	DEBUGPrintNumber(col, row, addr, 5, colour);
}

static void DEBUGHighlightRow(int row, int xPos, int w) {
	SDL_Rect rc;
	rc.w = w;
	rc.h = layout.rowHeight;
	rc.x = xPos;
	rc.y = row * layout.rowHeight - 1;
	SDL_SetRenderDrawColor(dbgRenderer, col_highlight.r, col_highlight.g, col_highlight.b, 100);
	SDL_RenderFillRect(dbgRenderer, &rc);
}

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

	if(var_BPonBRK && (0 == real_read6502(pc, false, 0))) {
		currentPC = ++pc;
		currentMode = DMODE_STOP;
	}

	if (currentMode == DMODE_STEP) {							// Single step before
		currentPC = pc;											// Update current PC
		currentMode = DMODE_STOP;								// So now stop, as we've done it.
	}

	if ((breakpointsCount && DEBUGisOnBreakpoint(pc, BPT_PC)) || pc == stepBreakPoint) {				// Hit a breakpoint.
		// to allow scroll code while on BP
		if(currentMode != DMODE_STOP)
			currentPC = pc;											// Update current PC
		currentMode = DMODE_STOP;								// So now stop, as we've done it.
		stepBreakPoint = -1;									// Clear step breakpoint.
		currentPCBank= -1;
	}

	if (SDL_GetKeyboardState(NULL)[DBGSCANKEY_BRK]) {			// Stop on break pressed.
		currentMode = DMODE_STOP;
		currentPC = pc; 										// Set the PC to what it is.
	}

	if(currentPC < 0)
		currentPC = pc;

	if(currentPCBank < 0)
		currentPCBank= currentPC < 0xC000 ? memory_get_ram_bank() : memory_get_rom_bank();

	if (currentMode != DMODE_RUN) {								// Not running, we own the keyboard.
		while (SDL_PollEvent(&event)) { 						// We now poll events here.

			switch(event.type) {
				case SDL_WINDOWEVENT:
					if(event.window.event == SDL_WINDOWEVENT_CLOSE)
						return -1;
					break;

				case SDL_QUIT:									// Time for exit
					return -1;

				case SDL_KEYDOWN:								// Handle key presses.
				case SDL_TEXTINPUT:
				{
					int isHandled= DEBUGHandleKeyEvent(&event);
					if(isHandled)
						continue;
					break;
				}

				case SDL_MOUSEMOTION:
				{
					if(dbgWindowID != event.motion.windowID)
						break;

					mouseZone= MZ_NONE;
					SDL_Point mouse_position= {event.motion.x, event.motion.y};
					for(int idx= MZ_NONE+1; idx < mouseZonesCount; idx++) {
						if(SDL_PointInRect(&mouse_position, mouseZones[idx].rect)) {
							mouseZone= idx;
							break;
						}
					}

					break;
				}

				case SDL_MOUSEWHEEL:
				{
					if(dbgWindowID != event.wheel.windowID || !event.wheel.y)
						break;

					switch(mouseZone) {
						case MZ_CODE:
						{
							int inc= (event.wheel.y > 0) ? -3 : disasmLine1Size;
							currentPC+= inc;
							break;
						}
						case MZ_DATA:
						{
							int inc= layout.dataLinecount * 16 * ((event.wheel.y > 0) ? -1 : 1);
							currentData = (currentData + inc) & (dumpmode == DDUMP_RAM ? 0xFFFF : 0x1FFFF);
							break;
						}
						case MZ_STACK:
						{
							offsetSP= (offsetSP + (event.wheel.y > 0 ? -1 : 1)) % 255;
							break;
						}
					}
					break;

				}

			}

			CON_Events(&event);

		}
	}

	showDebugOnRender = (currentMode != DMODE_RUN);				// Do we draw it - only in RUN mode.
	if (currentMode == DMODE_STOP) { 							// We're in charge.
		video_update();

		if(!isWindowVisible) {
			SDL_ShowWindow(dbgWindow);
			isWindowVisible= 1;
		}

		SDL_RenderClear(dbgRenderer);
		DEBUGRenderDisplay(win_width, win_height);
		SDL_RenderPresent(dbgRenderer);
		return 1;
	}

	if (currentMode != DMODE_STEP) {
		SDL_HideWindow(dbgWindow);
		isWindowVisible= 0;
	}

	return 0;													// Run wild, run free.
}

// *******************************************************************************************
//
//								Setup fonts and co
//
// *******************************************************************************************
void DEBUGsetFont(int fontNumber) {
	dbgFontID= fontNumber;

	int charWidth= DT_FontWidth(dbgFontID);
	int charHeight= DT_FontHeight(dbgFontID);

	// adjust focus rect width to follow font char width
	// smallCodeZoneRect.w=
	// largeCodeZoneRect.w= charWidth * DBG_LAYOUT_CODE_WIDTH;

	// adjust focus rect width to follow font char width
	smallDataZoneRect.w= charWidth * (DBG_LAYOUT_DATA_WIDTH+1);
	// smallDataZoneRect.y= (smallDataZoneRect.y / charHeight) * charHeight;
	// smallDataZoneRect.h= win_height - con_height - smallDataZoneRect.y - 5;

	layout.totalHeight= win_height / charHeight;
	layout.totalWidth= win_width / charWidth;
	layout.rowHeight= charHeight + 1;

	DEBUGupdateLayout(-1);

}

/*
*/
bool DEBUGreadColour(const dictionary *dict, const char *key, SDL_Colour *col) {
	int val= iniparser_getint(dict, key, -1);
	if(val>0) {
		col->r= val >> 16;
		col->g= val >> 8 & 0xFF;
		col->b= val & 0xFF;
		return true;
	}
	return false;
}

/*
*/
void DEBUGiniScriptExecLine(const dictionary *dict, const char *key, const char *entry) {
	(void)entry;
	const char *cmd= iniparser_getstring(dict, key, "");
	CON_Out(console, cmd);
	DEBUG_Command_Handler(console, (char *)cmd);
}

/*
*/
void DEBUGsetKeyBinding(const dictionary *dict, const char *key, const char *entry) {
	char keyName[256];
	const char *subStr;
	char *sep;
	SDL_Keymod mods= 0;
	SDL_Keycode keycode;
	const char *binding= iniparser_getstring(dict, key, "");

	*keyName= 0;
	subStr= binding;
	for(int idx= 0; keyBindings[idx].binding; idx++) {
		if(!stricmp(keyBindings[idx].name, entry)) {

			do {
				sep= strstr(subStr, "::");

				if(!strnicmp("SHIFT", subStr, 5))
					mods |= KMOD_SHIFT;
				else
				if(!strnicmp("CTRL", subStr, 4))
					mods |= KMOD_CTRL;
				else
				if(!strnicmp("ALT", subStr, 3))
					mods |= KMOD_ALT;
				else
				if(!strnicmp("GUI", subStr, 3))
					mods |= KMOD_GUI;
				else
					strncpy(keyName, subStr, sep != NULL ? sep - subStr : sizeof keyName);

				subStr= sep+2;

			} while(sep != NULL);

			keycode= SDL_GetKeyFromName(ltrim(keyName));
			if(keycode == SDLK_UNKNOWN) {
				CON_Out(console, "%sERR: unkown binding for %s %s", DT_color_red, entry, DT_color_default);
				return;
			}

			keyBindings[idx].key= keycode;
			keyBindings[idx].keyMod= mods;
		}
	}

}

/*
*/
void DEBUGreadSettings(dictionary *iniDict) {

	iniparser_foreachkeys(iniDict, "dbg_ini_script", DEBUGiniScriptExecLine);

	iniparser_foreachkeys(iniDict, "key_bindings", DEBUGsetKeyBinding);

	char buffer[32];
	char *bp;
	for(int idx= 0; idx < DBG_MAX_BREAKPOINTS; idx++) {
		sprintf(buffer, "dbg:BP%d", idx);
		bp= (char *)iniparser_getstring(iniDict, buffer, NULL);
		if(bp && bp[0] != '\0') {
			sprintf(buffer, "bp %s", bp);
			CON_Out(console, buffer);
			DEBUG_Command_Handler(console, buffer);
		}
	}

	int fontNum= DT_LoadFont(dbgRenderer, iniparser_getstring(iniDict, "dbg:font", "console9.bmp"), TRANS_FONT);
	DEBUGsetFont(fontNum>=0 ? fontNum : 0);

	DEBUGreadColour(iniDict, "dbg:focus_color", &col_dbg_focus);
	DEBUGreadColour(iniDict, "dbg:bg_color", &col_dbg_bkgnd);

	DEBUGreadColour(iniDict, "dbg:label_color", &col_label);
	DEBUGreadColour(iniDict, "dbg:data_color", &col_data);
	DEBUGreadColour(iniDict, "dbg:highlight_color", &col_highlight);

	DEBUGreadColour(iniDict, "dbg:vram_tilemap_color", &col_vram[0]);
	DEBUGreadColour(iniDict, "dbg:vram_tiledata_color", &col_vram[1]);
	DEBUGreadColour(iniDict, "dbg:vram_special_color", &col_vram[2]);
	DEBUGreadColour(iniDict, "dbg:vram_other_color", &col_vram[3]);

	DEBUGreadColour(iniDict, "dbg_console:bg_color", &col_con_bkgnd);
	CON_BackgroundColor(console, col_con_bkgnd.r, col_con_bkgnd.g, col_con_bkgnd.b);

	wannaShowMouseCoord= 1 == iniparser_getboolean(iniDict, "dbg:showMouseCoord", 0);
}

/*
*/
void DEBUGInitUI(SDL_Renderer *pRenderer) {
	(void)pRenderer;

	SDL_Rect Con_rect;

	dictionary *iniDict= iniparser_load("x16emu.ini");

	dbgWindow= SDL_CreateWindow("X16 Debugger", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, win_width, win_height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
	dbgRenderer= SDL_CreateRenderer(dbgWindow, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED );
	SDL_SetRenderDrawBlendMode(dbgRenderer, SDL_BLENDMODE_BLEND);
	dbgWindowID= SDL_GetWindowID(dbgWindow);
	SDL_ShowCursor(SDL_ENABLE);

	// DEBUGInitChars(pRenderer);
	// dbgRenderer = pRenderer;				// Save renderer.

	char *fontPath= (char *)iniparser_getstring(iniDict, "dbg_console:font", "console9.bmp");

	Con_rect.x = 0;
	Con_rect.y = 0;
	Con_rect.w = win_width;
	Con_rect.h = win_height;
	console= CON_Init(fontPath, dbgRenderer, 50, Con_rect);
	CON_SetExecuteFunction(console, DEBUG_Command_Handler);

	commands_init();

	DEBUGreadSettings(iniDict);

	CON_Show(console);

	DEBUGupdateLayout(0);

	iniparser_freedict(iniDict);

}

// *******************************************************************************************
//
//
//
// *******************************************************************************************
void DEBUGFreeUI() {
	CON_Destroy(console);
	commands_free();
}

// *******************************************************************************************
//
//								Add a new breakpoint address
//
// *******************************************************************************************

void DEBUGSetBreakPoint(int newBreakPoint) {
	char command[16];
	sprintf(command, "bp %x", newBreakPoint);
	DEBUG_Command_Handler(console, command);
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
int DEBUGGetKeyBinding(SDL_Keysym *key) {
	for(int idx= 0; keyBindings[idx].binding; idx++) {
		if((key->sym == keyBindings[idx].key) && (key->mod & keyBindings[idx].keyMod))
			return keyBindings[idx].binding;
	}
	return 0;
}

static int DEBUGHandleKeyEvent(SDL_Event *event) {
	int opcode;

	switch(event->type) {
		case SDL_KEYDOWN:
			switch(DEBUGGetKeyBinding(&(event->key.keysym))) {

				case DBGKEY_STEP:									// Single step (F11 by default)
					currentMode = DMODE_STEP; 						// Runs once, then switches back.
					currentPCBank= -1;
					break;

				case DBGKEY_STEPOVER:								// Step over (F10 by default)
					opcode = real_read6502(pc, false, 0);							// What opcode is it ?
					if (opcode == 0x20) { 							// Is it JSR ?
						stepBreakPoint = pc + 3;					// Then break 3 on.
						currentMode = DMODE_RUN;					// And run.
					} else {
						currentMode = DMODE_STEP;					// Otherwise single step.
						currentPCBank= -1;
					}
					break;

				case DBGKEY_RUN:									// F5 Runs until Break.
					currentMode = DMODE_RUN;
					break;

				case DBGKEY_SETBRK:									// F9 Set breakpoint to displayed.
					DEBUGSetBreakPoint(currentPC);
					break;

				case DBGKEY_HOME:									// F1 sets the display PC to the actual one.
					currentPC = pc;
					currentPCBank= currentPC < 0xC000 ? memory_get_ram_bank() : memory_get_rom_bank();
					offsetSP= 0;
					break;

				case DBGKEY_RESET:									// F2 reset the 6502
					reset6502();
					currentPC = pc;
					currentPCBank= -1;
					break;

				case DBGKEY_PASTE:
				{
					char *text= SDL_GetClipboardText();
					if(text) {
						Cursor_Paste(console, text);
						SDL_free(text);
					}
					break;
				}
			}
			break;

		case SDL_TEXTINPUT:
			switch(event->text.text[0]) {
				case '+':
					if(mouseZone == MZ_DATA) {
						currentBank += 1;
						return 1;
					}

				case '-':
					if(mouseZone == MZ_DATA) {
						currentBank -= 1;
						return 1;
					}

			}
			break;
	}

	return 0;

}

// *******************************************************************************************
//
//							Commands Handler
//
// *******************************************************************************************

void DEBUG_Command_Handler(ConsoleInformation *console, char* command) {
	int argc;
	char* argv[128];
	char* linecopy;

	linecopy= strdup(command);
	argc= splitline(argv, (sizeof argv)/(sizeof argv[0]), linecopy);
	if(!argc) {
		free(linecopy);
		showFullConsole= 1-showFullConsole;
		return;
	}

	for (int idx= 0; cmd_table[idx].name; idx++) {
		if(!strcasecmp(cmd_table[idx].name, argv[0])) {
			if(argc-1 < cmd_table[idx].minargc) {
				CON_Out(console, cmd_table[idx].help);
				return;
			}
			cmd_table[idx].fn(cmd_table[idx].data, argc, argv);
			return;
		}
	}

	CON_Out(console, "%sERR: unknown command%s", DT_color_red, DT_color_default);
}

// *******************************************************************************************
//
//									 Render Data Area
//
// *******************************************************************************************
#define DATA_ADDR_WIDTH 2+1+4 + 1
#define DATA_BYTES_WIDTH 16*3
static void DEBUGRenderData(int col, int row, int lineCount) {
	int memaddr= currentData;

	while(lineCount--) {									// To bottom of screen
		memaddr &= 0xFFFF;
		DEBUGPrintAddress(col, row, (uint8_t)currentBank, memaddr, col_label);	// Show label.
		for (int i = 0;i < 16;i++) {
			int addr= memaddr + i;
			// if in RAM or in ROM and outside existing banks, print nothing
			if(!isValidAddr(currentBank, addr))
				continue;
			else {
				int byte= real_read6502(addr, true, currentBank);
				DEBUGPrintNumber(col + DATA_ADDR_WIDTH+i*3, row, byte, 2, col_data);
				DEBUGPrintChar(dbgRenderer, col + DATA_ADDR_WIDTH+DATA_BYTES_WIDTH+i, row, byte==0 ? '.' : byte, col_data);
			}
		}
		row++;
		memaddr+= 16;
	}

}

static void
DEBUGRenderVRAM(int col, int row, int lineCount)
{
	int vmemaddr= currentData;
	while (lineCount--) {
		DEBUGPrintVAddress(col, row, vmemaddr & 0x1FFFF, col_label); // Show label.

		for (int i = 0; i < 16; i++) {
			int addr = (vmemaddr + i) & 0x1FFFF;
			int byte = video_space_read(addr);

			int type= video_get_address_type(addr) % col_vram_len;
			DEBUGPrintNumber(col + 6 + i * 3, row, byte, 2, col_vram[type]);
		}
		row++;
		vmemaddr += 16;
	}
}

// *******************************************************************************************
//
//									 Render Disassembly
//
// *******************************************************************************************
#define CODE_ADDR_WIDTH 2+1+4 + 1
#define CODE_BYTES_WIDTH 2+1+2+1+2 + 1
#define CODE_LABEL_WIDTH SYMBOL_LABEL_MAXLEN + 1
static void DEBUGRenderCode(int col, int row, int lineCount) {
	char buffer[48];
	char *label;
	int initialPC= currentPC - 18;// - lineCount;
	int lineSize[18];
	int lineSizeCount= -1;

	// HACK: rollup a bit in hope to sync with proper disasm ;)
	while(initialPC < currentPC) {
		int size = disasm(initialPC, RAM, buffer, sizeof(buffer), currentPCBank);
		initialPC += size;
		lineSize[++lineSizeCount]= size;
	}
	currentPC= initialPC;
	for(int idx = 0; (idx < 8)&&(lineSizeCount >= 0); idx++, lineSizeCount--) {
		initialPC-= lineSize[lineSizeCount];
	}

	for (; row < lineCount; row++) { 							// Each line

		label= symbol_find_label(currentPCBank, initialPC);
		if(label) {
			DEBUGPrintString(dbgRenderer, col, row++, label, *label != '@' ? col_dis_fnlabel : col_dis_sublabel);
		}

		DEBUGPrintAddress(col, row, currentPCBank, initialPC, col_label);

		if(!isValidAddr(currentPCBank, initialPC)) {
			initialPC++;
			continue;
		}

		int size = disasm(initialPC, RAM, buffer, sizeof(buffer), currentPCBank);	// Disassemble code
		if(row == 0)
			disasmLine1Size= size;

		if(initialPC == pc) {
			DEBUGHighlightRow(row, mouseZones[MZ_CODE].rect->x, mouseZones[MZ_CODE].rect->w );
		}
		DEBUGPrintString(dbgRenderer, col+CODE_ADDR_WIDTH+CODE_BYTES_WIDTH+CODE_LABEL_WIDTH, row, buffer, col_data);

		for(int byteCount= 0; byteCount<size;byteCount++) {
			int byte= real_read6502(initialPC + byteCount, true, currentPCBank);
			DEBUGPrintNumber(col+CODE_ADDR_WIDTH+byteCount*3, row, byte, 2, col_data);
		}

		initialPC += size;										// Forward to next
	}

}

// *******************************************************************************************
//
//									Render Register Display
//
// *******************************************************************************************

static TRegisterPos regs[]= {
	{ KIND_CPU, REG_P,   2, 0, { "P", 0, 0}, {0, 1} },
	{ KIND_CPU, REG_P,  -1, 0, { "NVRBDIZC", 3, 0}, {3, 1} },
	{ KIND_CPU, REG_A,   2, 1, { "A",  0, 2}, {3, 2} },
	{ KIND_CPU, REG_X,   2, 1, { "X",  0, 3}, {3, 3} },
	{ KIND_CPU, REG_Y,   2, 1, { "Y",  0, 4}, {3, 4} },
	{ KIND_CPU, REG_PC,  4, 0, { "PC", 0, 5}, {3, 5} },
	{ KIND_CPU, REG_SP,  4, 0, { "SP", 0, 6}, {3, 6} },

	{ KIND_CPU, REG_BKA, 2, 0, { "BKA", 0, 8}, {4, 8} },
	{ KIND_CPU, REG_BKO, 2, 0, { "BKO", 7, 8}, {11, 8} },

	{ KIND_CPU, REG_VA,  6, 0, { "VA",  0, 10}, {3, 10} },
	{ KIND_CPU, REG_VD0, 2, 0, { "VD0", 0, 11}, {0, 12} },
	{ KIND_CPU, REG_VD1, 2, 0, { "VD1", 4, 11}, {4, 12} },
	{ KIND_CPU, REG_VCT, 2, 0, { "VCT", 8, 11}, {8, 12} },

	{ KIND_ZP, REG_R0,  4, 0, { "R0",  0, 0}, {4, 0} },
	{ KIND_ZP, REG_R1,  4, 0, { "R1",  0, 1}, {4, 1} },
	{ KIND_ZP, REG_R2,  4, 0, { "R2",  0, 2}, {4, 2} },
	{ KIND_ZP, REG_R3,  4, 0, { "R3",  0, 3}, {4, 3} },

	{ KIND_ZP, REG_R4,  4, 0, { "R4",  0, 5}, {4, 5} },
	{ KIND_ZP, REG_R5,  4, 0, { "R5",  0, 6}, {4, 6} },
	{ KIND_ZP, REG_R6,  4, 0, { "R6",  0, 7}, {4, 7} },
	{ KIND_ZP, REG_R7,  4, 0, { "R7",  0, 8}, {4, 8} },

	{ KIND_ZP, REG_R8,  4, 0, { "R8",  0, 10}, {4, 10} },
	{ KIND_ZP, REG_R9,  4, 0, { "R9",  0, 11}, {4, 11} },
	{ KIND_ZP, REG_R10, 4, 0, { "R10", 0, 12}, {4, 12} },
	{ KIND_ZP, REG_R11, 4, 0, { "R11", 0, 13}, {4, 13} },

	{ KIND_ZP, REG_R12, 4, 0, { "R12", 0, 15}, {4, 15} },
	{ KIND_ZP, REG_R13, 4, 0, { "R13", 0, 16}, {4, 16} },
	{ KIND_ZP, REG_R14, 4, 0, { "R14", 0, 17}, {4, 17} },
	{ KIND_ZP, REG_R15, 4, 0, { "R15", 0, 18}, {4, 18} },

	{ KIND_ZP, REG_x16, 4, 0, { "x16", 0, 20}, {4, 20} },
	{ KIND_ZP, REG_x17, 4, 0, { "x17", 0, 21}, {4, 21} },
	{ KIND_ZP, REG_x18, 4, 0, { "x18", 0, 22}, {4, 22} },
	{ KIND_ZP, REG_x19, 4, 0, { "x19", 0, 23}, {4, 23} },

	{ KIND_BP, REG_BP0, 6, 0, { "B%c0", 0, 0}, {4, 0} },
	{ KIND_BP, REG_BP1, 6, 0, { "B%c1", 0, 0+1}, {4, 0+1} },
	{ KIND_BP, REG_BP2, 6, 0, { "B%c2", 0, 0+2}, {4, 0+2} },
	{ KIND_BP, REG_BP3, 6, 0, { "B%c3", 0, 0+3}, {4, 0+3} },
	{ KIND_BP, REG_BP4, 6, 0, { "B%c4", 0, 0+4}, {4, 0+4} },
	{ KIND_BP, REG_BP5, 6, 0, { "B%c5", 0, 0+5}, {4, 0+5} },
	{ KIND_BP, REG_BP6, 6, 0, { "B%c6", 0, 0+6}, {4, 0+6} },
	{ KIND_BP, REG_BP7, 6, 0, { "B%c7", 0, 0+7}, {4, 0+7} },
	{ KIND_BP, REG_BP8, 6, 0, { "B%c8", 0, 0+8}, {4, 0+8} },
	{ KIND_BP, REG_BP9, 6, 0, { "B%c9", 0, 0+9}, {4, 0+9} },

	{ 0, 0, 0, 0, { NULL, 0, 0}, {0, 0} }
};

int DEBUGreadVirtualRegister(int reg) {
	int reg_addr = 2 + reg * 2;
	return real_read6502(reg_addr+1, true, currentBank)*256+real_read6502(reg_addr, true, currentBank);
}

static void DEBUGRenderRegisters(int col, int row, TRegister_kind regKing) {
	int value= 0;
	char buffer[10];

	for(int idx= 0; regs[idx].regCode; idx++) {

		if(regKing != regs[idx].regKind)
			continue;

		TRegisterLabelPos *labelPos= &regs[idx].label;
		TRegisterValuePos *valuePos= &regs[idx].value;
		char *label= labelPos->text;

		switch(regs[idx].regCode) {
			case REG_A: value= a; break;
			case REG_X: value= x; break;
			case REG_Y: value= y; break;
			case REG_P: value= status; break;
			case REG_PC: value= pc; break;
			case REG_SP: value= sp|0x100; break;
			case REG_BKA: value= memory_get_ram_bank(); break;
			case REG_BKO: value= memory_get_rom_bank(); break;
			case REG_VA: value= video_read(0, true) | (video_read(1, true)<<8) | (video_read(2, true)<<16); break;
			case REG_VD0: value= video_read(3, true); break;
			case REG_VD1: value= video_read(4, true); break;
			case REG_VCT: value= video_read(5, true); break;

			case REG_R0: case REG_R1: case REG_R2: case REG_R3:
			case REG_R4: case REG_R5: case REG_R6: case REG_R7:
			case REG_R8: case REG_R9: case REG_R10: case REG_R11:
			case REG_R12: case REG_R13: case REG_R14: case REG_R15:
			case REG_x16: case REG_x17: case REG_x18:
			case REG_x19:
				value= DEBUGreadVirtualRegister(regs[idx].regCode - REG_R0);
				break;

			case REG_BP0: case REG_BP1: case REG_BP2: case REG_BP3:
			case REG_BP4: case REG_BP5: case REG_BP6: case REG_BP7:	case REG_BP8:
			case REG_BP9:
			{
				int bpIdx= regs[idx].regCode - REG_BP0;
				if(bpIdx >= breakpointsCount)
					continue;
				value= breakpoints[bpIdx].addr;
				char typeCh;
				switch(breakpoints[bpIdx].type) {
					default: 		typeCh= 'p'; break;
					case BPT_MEM: 	typeCh= 'm'; break;
					case BPT_RMEM: 	typeCh= 'r'; break;
					case BPT_WMEM: 	typeCh= 'w'; break;
				}
				sprintf(buffer, label, typeCh);
				label= buffer;
				break;
			}
		}

		if(col<0)
			col+= layout.totalWidth;

		DEBUGPrintString(dbgRenderer, labelPos->xOffset + col, labelPos->yOffset + row, label, col_label);
		DEBUGPrintNumber(valuePos->xOffset + col, valuePos->yOffset + row, value, regs[idx].width, col_data);
		if(regs[idx].showChar)
			DEBUGPrintChar(dbgRenderer, valuePos->xOffset + col + 3, valuePos->yOffset + row, value, col_data);
	}

}

// *******************************************************************************************
//
//									Render Top of Stack
//
// *******************************************************************************************

static void DEBUGRenderStack(int bytesCount) {
	int data= ((sp-6+offsetSP) & 0xFF) | 0x100;
	int row= 0;
	int xOffset= layout.stackCol < 0 ? layout.totalWidth + layout.stackCol : layout.stackCol;
	while (row < bytesCount) {

		if((data & 0xFF) == sp) {
			DEBUGHighlightRow(row, mouseZones[MZ_STACK].rect->x, mouseZones[MZ_STACK].rect->h);
		}

		DEBUGPrintNumber(xOffset, row, data & 0xFFFF, 4, col_label);
		int byte = real_read6502((data++) & 0xFFFF, false, 0);
		DEBUGPrintNumber(xOffset+5, row, byte, 2, col_data);
		DEBUGPrintChar(dbgRenderer, xOffset+8, row, byte, col_data);
		row++;
		data= (data & 0xFF) | 0x100;
	}
}

// *******************************************************************************************
//
//							Render the emulator debugger display.
//
// *******************************************************************************************
void DEBUGupdateLayout(int id) {
	int ch= DT_FontHeight(dbgFontID)+1;
	int cw= DT_FontWidth(dbgFontID);

	if(id>=0)
		layoutID= id;

	switch(layoutID) {
		case 2:
			mouseZones[MZ_DATA].rect= &largeDataZoneRect;
			mouseZones[MZ_CODE].rect= &emptyZoneRect;
			layout.dataLinecount= mouseZones[MZ_DATA].rect->h / ch;
			layout.dataRow= 0;
			layout.codeLinecount= 0;
			layout.zpRegLinecount= 0;
			layout.regLinecount= 0;
			layout.bpRegLinecount= 0;
			break;

		case 1:
			mouseZones[MZ_DATA].rect= &emptyZoneRect;
			mouseZones[MZ_CODE].rect= &largeCodeZoneRect;
			layout.codeLinecount= mouseZones[MZ_CODE].rect->h / ch;
			layout.dataLinecount= 0;
			layout.regLinecount= 1;
			layout.bpRegLinecount= 1;
			layout.zpRegLinecount= 20;
			break;

		default:
		{
			mouseZones[MZ_DATA].rect= &smallDataZoneRect;
			mouseZones[MZ_CODE].rect= &smallCodeZoneRect;

			layout.codeLinecount= mouseZones[MZ_CODE].rect->h / ch;

			layout.dataLinecount= mouseZones[MZ_DATA].rect->h / ch;
			layout.dataRow= layout.codeLinecount+1;

			layout.stackLinecount= mouseZones[MZ_STACK].rect->h / ch;
			layout.stackCol= -DBG_LAYOUT_STCK_WIDTH;

			layout.regCol= (mouseZones[MZ_CODE].rect->x + mouseZones[MZ_CODE].rect->w) / cw + 1;
			layout.regLinecount= 1;

			layout.bpRegCol= layout.regCol + DBG_LAYOUT_REG_WIDTH;
			layout.bpRegLinecount= 1;

			layout.zpRegCol= -DBG_LAYOUT_ZP_REG_WIDTH;
			layout.zpRegRow= ceil((double)mouseZones[MZ_ZPREG].rect->y / (double)ch);
			layout.zpRegLinecount= 20;

		}
	}
}

void DEBUGRenderDisplay(int width, int height) {

	CON_DrawConsole(console);

	if(showFullConsole)
		return;

	SDL_Rect rc;
	rc.w = width;
	rc.h = height - con_height + 2;
	rc.x = rc.y = 0;
	SDL_SetRenderDrawColor(dbgRenderer, col_dbg_bkgnd.r, col_dbg_bkgnd.g, col_dbg_bkgnd.b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(dbgRenderer, &rc);

	// Draw register name and values.
	if(layout.regLinecount)
		DEBUGRenderRegisters(layout.regCol, 0, KIND_CPU);
	if(layout.bpRegLinecount)
		DEBUGRenderRegisters(layout.bpRegCol, 0, KIND_BP);
	if(layout.zpRegLinecount)
		DEBUGRenderRegisters(layout.zpRegCol, layout.zpRegRow, KIND_ZP);

	if(layout.codeLinecount)
		DEBUGRenderCode(1, 0, layout.codeLinecount);

	if(layout.dataLinecount) {
		if (dumpmode == DDUMP_RAM)
			DEBUGRenderData(1, layout.dataRow, layout.dataLinecount);
		else
			DEBUGRenderVRAM(1, layout.dataRow, layout.dataLinecount);
	}

	if(layout.stackLinecount)
		DEBUGRenderStack(layout.stackLinecount);

	if(mouseZones[mouseZone].type == MZT_HIGHLIGHT) {
		SDL_SetRenderDrawColor(dbgRenderer, col_dbg_focus.r, col_dbg_focus.g, col_dbg_focus.b, 80);
		SDL_RenderFillRect(dbgRenderer, mouseZones[mouseZone].rect) ;
	}

	if(wannaShowMouseCoord) {
		char mouseCoord[30];
		int mouseX, mouseY;
		SDL_GetMouseState(&mouseX, &mouseY);
		sprintf(mouseCoord, "%03d %03d", mouseX, mouseY);
		DT_DrawText2(dbgRenderer, mouseCoord, dbgFontID, win_width-DT_FontWidth(dbgFontID)*9, win_height - con_height, col_highlight);
	}

}
