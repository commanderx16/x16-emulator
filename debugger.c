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

static void DEBUGHandleKeyEvent(SDL_Keycode key,int isShift);

static void DEBUGWrite(int x,int y,int ch,int r,int g,int b);
static void DEBUGString(int x,int y,char *s,int r,int g,int b);
static void DEBUGNumber(int x,int y,int n,int w,int r,int g,int b);

static void DEBUGRenderData(int y,int data);
static int DEBUGRenderRegisters(void);
static void DEBUGRenderCode(int lines,int initialPC);

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

#define DBGSCANKEY_BRK 	SDL_SCANCODE_F12 						// F12 is break into running code.	
#define DBGSCANKEY_SHOW	SDL_SCANCODE_TAB 						// Show screen key.

																// *** MUST BE SCAN CODES ***

int showDebugOnRender = 0;										// Used to trigger rendering in video.c
int showFullDisplay = 0; 										// If non-zero show the whole thing.
int currentPC = -1;												// Current PC value.
int currentData = 0;											// Current data display address.
int currentMode = DMODE_RUN;									// Start running.
int breakPoint = -1; 											// User Break
int stepBreakPoint = -1;										// Single step break.
//
//		This flag controls 
//
int xPos = 0;													// Position of debug window
int yPos = 0;	

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

	if (currentMode != DMODE_RUN) {								// Not running, we own the keyboard.
		showFullDisplay = 										// Check showing screen.
					SDL_GetKeyboardState(NULL)[DBGSCANKEY_SHOW];
		while (SDL_PollEvent(&event)) { 						// We now poll events here.
			if (event.type == SDL_QUIT) return -1; 				// Time for exit
			if (event.type == SDL_KEYDOWN) {					// Handle key presses.	
				DEBUGHandleKeyEvent(event.key.keysym.sym,
									SDL_GetModState() & (KMOD_LSHIFT|KMOD_RSHIFT));		
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
//								Map keycodes to hexadecimal
// *******************************************************************************************

static SDL_Keycode keyToHex[] = { 
	SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7,  
	SDLK_8, SDLK_9, SDLK_a, SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f
};

// *******************************************************************************************
//
//									Handle keyboard state.
//
// *******************************************************************************************

static void DEBUGHandleKeyEvent(SDL_Keycode key,int isShift) {
	if (key == DBGKEY_STEP) {									// Single step (F11 by default)
		currentMode = DMODE_STEP; 								// Runs once, then switches back.
	}
	if (key == DBGKEY_STEPOVER) {								// Step over (F10 by default)
		int opcode = read6502(pc);								// What opcode is it ?
		if (opcode == 0x20) { 									// Is it JSR ?
			stepBreakPoint = pc + 3;							// Then break 3 on.
			currentMode = DMODE_RUN;							// And run.
		} else {
			currentMode = DMODE_STEP;							// Otherwise single step.
		}
	}
	if (key == DBGKEY_RUN) {									// F5 Runs until Break.
		currentMode = DMODE_RUN;
	}
	if (key == DBGKEY_SETBRK) {									// F9 Set breakpoint to displayed.
		breakPoint = currentPC;
	}
	if (key == DBGKEY_HOME) {									// F1 sets the display PC to the actual one.
		currentPC = pc;
	}
	if (key == DBGKEY_RESET) { 									// F2 reset the 6502
		reset6502();
		currentPC = pc;
	}
	for (int code = 0;code < 16;code++) { 						// Check hexadecimal.
		if (key == keyToHex[code]) {
			if (isShift) {
				currentData = ((currentData << 4) | code) & 0xFFFF;
			} else {
				currentPC = ((currentPC << 4) | code) & 0xFFFF;
			}
		}
	}
}

// *******************************************************************************************
//
//							Render the emulator debugger display.
//
// *******************************************************************************************

void DEBUGRenderDisplay(int width,int height,SDL_Renderer *pRenderer) {
	if (showFullDisplay) return;								// Not rendering debug.
	dbgRenderer = pRenderer;									// Save renderer.
	SDL_Rect rc;
	rc.w = DBG_WIDTH * 6 * CHAR_SCALE;							// Erase background, set up rect
	rc.h = DBG_HEIGHT * 8 * CHAR_SCALE;
	xPos = width-rc.w-4;yPos = 4; 								// Position screen
	rc.x = xPos;rc.y = yPos; 									// Set rectangle and black out.
	SDL_SetRenderDrawColor(pRenderer,0,0,0,SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(pRenderer,&rc);
	int depth = DEBUGRenderRegisters();							// Draw register name and values.
	DEBUGRenderCode(depth,currentPC);							// Render 6502 disassembly.
	DEBUGRenderData(depth+1,currentData);
}

// *******************************************************************************************
//
//									 Render Data Area
//
// *******************************************************************************************

static void DEBUGRenderData(int y,int data) {
	while (y < DBG_HEIGHT) {									// To bottom of screen
		DEBUGNumber(DBG_MEMX,y,data & 0xFFFF,4,COL_LABEL);		// Show label.	
		for (int i = 0;i < 8;i++) {
			int byte = read6502((data+i) & 0xFFFF);
			DEBUGNumber(DBG_MEMX+5+i*3,y,byte,2,COL_DATA);
			DEBUGWrite(DBG_MEMX+30+i,y,byte,COL_DATA);
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

static void DEBUGRenderCode(int lines,int initialPC) {
	char buffer[32];
	for (int y = 0;y < lines;y++) { 							// Each line
		DEBUGNumber(DBG_ASMX,y,initialPC,4,COL_LABEL);			// Output label
		int size = disasm(initialPC,RAM,buffer,sizeof(buffer));	// Disassemble code
		if (initialPC == pc) {									// Output assembly highlighting PC
			DEBUGString(DBG_ASMX+5,y,buffer,COL_HIGHLIGHT);
		} else {
			DEBUGString(DBG_ASMX+5,y,buffer,COL_DATA);		
		}
		initialPC += size;										// Forward to next
	}
}

// *******************************************************************************************
//
//									Render Register Display
//
// *******************************************************************************************

static char *labels[] = { "PC","A","X","Y","S","SP","BRK","N","V","B","I","D","Z","C", NULL };

static int DEBUGRenderRegisters(void) {
	int n = 0,yc = 0;
	while (labels[n] != NULL) {									// Labels
		DEBUGString(DBG_LBLX,n,labels[n],COL_LABEL);n++;
	}
	DEBUGNumber(DBG_DATX,yc++,pc,4,COL_DATA);					// Output registers
	DEBUGNumber(DBG_DATX,yc++,a,2,COL_DATA);
	DEBUGNumber(DBG_DATX,yc++,x,2,COL_DATA);
	DEBUGNumber(DBG_DATX,yc++,y,2,COL_DATA);
	DEBUGNumber(DBG_DATX,yc++,status,2,COL_DATA);
	DEBUGNumber(DBG_DATX,yc++,sp|0x100,4,COL_DATA);
	DEBUGNumber(DBG_DATX,yc++,breakPoint & 0xFFFF,4,COL_DATA);
	DEBUGNumber(DBG_DATX,yc++,(status >> 7) & 1,1,COL_DATA);
	DEBUGNumber(DBG_DATX,yc++,(status >> 6) & 1,1,COL_DATA);
	DEBUGNumber(DBG_DATX,yc++,(status >> 4) & 1,1,COL_DATA);
	DEBUGNumber(DBG_DATX,yc++,(status >> 3) & 1,1,COL_DATA);
	DEBUGNumber(DBG_DATX,yc++,(status >> 2) & 1,1,COL_DATA);
	DEBUGNumber(DBG_DATX,yc++,(status >> 1) & 1,1,COL_DATA);
	DEBUGNumber(DBG_DATX,yc++,(status >> 0) & 1,1,COL_DATA);

	return n; 													// Number of code display lines
}

// *******************************************************************************************
//
//										Simple 5 x 7 Font
//
// *******************************************************************************************

static unsigned char fontdata[] = {
    0x00, 0x00, 0x00, 0x00, 0x00,   // 0x20 (space)
	0x00, 0x00, 0x5F, 0x00, 0x00,   // 0x21 '!'
	0x00, 0x07, 0x00, 0x07, 0x00,   // 0x22 '"'
	0x14, 0x7F, 0x14, 0x7F, 0x14,   // 0x23 '#'
	0x24, 0x2A, 0x7F, 0x2A, 0x12,   // 0x24 '$'
	0x23, 0x13, 0x08, 0x64, 0x62,   // 0x25 '%'
	0x36, 0x49, 0x55, 0x22, 0x50,   // 0x26 '&'
	0x00, 0x05, 0x03, 0x00, 0x00,   // 0x27 '''
	0x00, 0x1C, 0x22, 0x41, 0x00,   // 0x28 '('
	0x00, 0x41, 0x22, 0x1C, 0x00,   // 0x29 ')'
	0x08, 0x2A, 0x1C, 0x2A, 0x08,   // 0x2A '*'
	0x08, 0x08, 0x3E, 0x08, 0x08,   // 0x2B '+'
	0x00, 0x50, 0x30, 0x00, 0x00,   // 0x2C ','
	0x08, 0x08, 0x08, 0x08, 0x08,   // 0x2D '-'
	0x00, 0x60, 0x60, 0x00, 0x00,   // 0x2E '.'
	0x20, 0x10, 0x08, 0x04, 0x02,   // 0x2F '/'
	0x3E, 0x51, 0x49, 0x45, 0x3E,   // 0x30 '0'
	0x00, 0x42, 0x7F, 0x40, 0x00,   // 0x31 '1'
	0x42, 0x61, 0x51, 0x49, 0x46,   // 0x32 '2'
	0x21, 0x41, 0x45, 0x4B, 0x31,   // 0x33 '3'
	0x18, 0x14, 0x12, 0x7F, 0x10,   // 0x34 '4'
	0x27, 0x45, 0x45, 0x45, 0x39,   // 0x35 '5'
	0x3C, 0x4A, 0x49, 0x49, 0x30,   // 0x36 '6'
	0x01, 0x71, 0x09, 0x05, 0x03,   // 0x37 '7'
	0x36, 0x49, 0x49, 0x49, 0x36,   // 0x38 '8'
	0x06, 0x49, 0x49, 0x29, 0x1E,   // 0x39 '9'
	0x00, 0x36, 0x36, 0x00, 0x00,   // 0x3A ':'
	0x00, 0x56, 0x36, 0x00, 0x00,   // 0x3B ';'
	0x00, 0x08, 0x14, 0x22, 0x41,   // 0x3C '<'
	0x14, 0x14, 0x14, 0x14, 0x14,   // 0x3D '='
	0x41, 0x22, 0x14, 0x08, 0x00,   // 0x3E '>'
	0x02, 0x01, 0x51, 0x09, 0x06,   // 0x3F '?'
	0x32, 0x49, 0x79, 0x41, 0x3E,   // 0x40 '@'
	0x7E, 0x11, 0x11, 0x11, 0x7E,   // 0x41 'A'
	0x7F, 0x49, 0x49, 0x49, 0x36,   // 0x42 'B'
	0x3E, 0x41, 0x41, 0x41, 0x22,   // 0x43 'C'
	0x7F, 0x41, 0x41, 0x22, 0x1C,   // 0x44 'D'
	0x7F, 0x49, 0x49, 0x49, 0x41,   // 0x45 'E'
	0x7F, 0x09, 0x09, 0x01, 0x01,   // 0x46 'F'
	0x3E, 0x41, 0x41, 0x51, 0x32,   // 0x47 'G'
	0x7F, 0x08, 0x08, 0x08, 0x7F,   // 0x48 'H'
	0x00, 0x41, 0x7F, 0x41, 0x00,   // 0x49 'I'
	0x20, 0x40, 0x41, 0x3F, 0x01,   // 0x4A 'J'
	0x7F, 0x08, 0x14, 0x22, 0x41,   // 0x4B 'K'
	0x7F, 0x40, 0x40, 0x40, 0x40,   // 0x4C 'L'
	0x7F, 0x02, 0x04, 0x02, 0x7F,   // 0x4D 'M'
	0x7F, 0x04, 0x08, 0x10, 0x7F,   // 0x4E 'N'
	0x3E, 0x41, 0x41, 0x41, 0x3E,   // 0x4F 'O'
	0x7F, 0x09, 0x09, 0x09, 0x06,   // 0x50 'P'
	0x3E, 0x41, 0x51, 0x21, 0x5E,   // 0x51 'Q'
	0x7F, 0x09, 0x19, 0x29, 0x46,   // 0x52 'R'
	0x46, 0x49, 0x49, 0x49, 0x31,   // 0x53 'S'
	0x01, 0x01, 0x7F, 0x01, 0x01,   // 0x54 'T'
	0x3F, 0x40, 0x40, 0x40, 0x3F,   // 0x55 'U'
	0x1F, 0x20, 0x40, 0x20, 0x1F,   // 0x56 'V'
	0x7F, 0x20, 0x18, 0x20, 0x7F,   // 0x57 'W'
	0x63, 0x14, 0x08, 0x14, 0x63,   // 0x58 'X'
	0x03, 0x04, 0x78, 0x04, 0x03,   // 0x59 'Y'
	0x61, 0x51, 0x49, 0x45, 0x43,   // 0x5A 'Z'
	0x00, 0x00, 0x7F, 0x41, 0x41,   // 0x5B '['
	0x02, 0x04, 0x08, 0x10, 0x20,   // 0x5C '\'
	0x41, 0x41, 0x7F, 0x00, 0x00,   // 0x5D ']'
	0x04, 0x02, 0x01, 0x02, 0x04,   // 0x5E '^'
	0x40, 0x40, 0x40, 0x40, 0x40,   // 0x5F '_'
	0x00, 0x01, 0x02, 0x04, 0x00,   // 0x60 '`'
	0x20, 0x54, 0x54, 0x54, 0x78,   // 0x61 'a'
	0x7F, 0x48, 0x44, 0x44, 0x38,   // 0x62 'b'
	0x38, 0x44, 0x44, 0x44, 0x20,   // 0x63 'c'
	0x38, 0x44, 0x44, 0x48, 0x7F,   // 0x64 'd'
	0x38, 0x54, 0x54, 0x54, 0x18,   // 0x65 'e'
	0x08, 0x7E, 0x09, 0x01, 0x02,   // 0x66 'f'
	0x08, 0x14, 0x54, 0x54, 0x3C,   // 0x67 'g'
	0x7F, 0x08, 0x04, 0x04, 0x78,   // 0x68 'h'
	0x00, 0x44, 0x7D, 0x40, 0x00,   // 0x69 'i'
	0x20, 0x40, 0x44, 0x3D, 0x00,   // 0x6A 'j'
	0x00, 0x7F, 0x10, 0x28, 0x44,   // 0x6B 'k'
	0x00, 0x41, 0x7F, 0x40, 0x00,   // 0x6C 'l'
	0x7C, 0x04, 0x18, 0x04, 0x78,   // 0x6D 'm'
	0x7C, 0x08, 0x04, 0x04, 0x78,   // 0x6E 'n'
	0x38, 0x44, 0x44, 0x44, 0x38,   // 0x6F 'o'
	0x7C, 0x14, 0x14, 0x14, 0x08,   // 0x70 'p'
	0x08, 0x14, 0x14, 0x18, 0x7C,   // 0x71 'q'
	0x7C, 0x08, 0x04, 0x04, 0x08,   // 0x72 'r'
	0x48, 0x54, 0x54, 0x54, 0x20,   // 0x73 's'
	0x04, 0x3F, 0x44, 0x40, 0x20,   // 0x74 't'
	0x3C, 0x40, 0x40, 0x20, 0x7C,   // 0x75 'u'
	0x1C, 0x20, 0x40, 0x20, 0x1C,   // 0x76 'v'
	0x3C, 0x40, 0x30, 0x40, 0x3C,   // 0x77 'w'
	0x44, 0x28, 0x10, 0x28, 0x44,   // 0x78 'x'
	0x0C, 0x50, 0x50, 0x50, 0x3C,   // 0x79 'y'
	0x44, 0x64, 0x54, 0x4C, 0x44,   // 0x7A 'z'
	0x00, 0x08, 0x36, 0x41, 0x00,   // 0x7B '{'
	0x00, 0x00, 0x7F, 0x00, 0x00,   // 0x7C '|'
	0x00, 0x41, 0x36, 0x08, 0x00,   // 0x7D '}'
	0x08, 0x08, 0x2A, 0x1C, 0x08,   // 0x7E ->'
	0x08, 0x1C, 0x2A, 0x08, 0x08,   // 0x7F <-'
};

// *******************************************************************************************
//
//									Write Hex Constant
//
// *******************************************************************************************

static void DEBUGNumber(int x,int y,int n,int w,int r,int g,int b) {
	char fmtString[8],buffer[16];
	snprintf(fmtString,sizeof(fmtString),"%%0%dx",w);
	snprintf(buffer,sizeof(buffer),fmtString,n);
	DEBUGString(x,y,buffer,r,g,b);

}

// *******************************************************************************************
//
//										Write String
//
// *******************************************************************************************

static void DEBUGString(int x,int y,char *s,int r,int g,int b) {
	while (*s != '\0') {
		DEBUGWrite(x++,y,*s++,r,g,b);
	}
}
// *******************************************************************************************
//
//										Write character
//
// *******************************************************************************************

static void DEBUGWrite(int x,int y,int ch,int r,int g,int b) {
	SDL_Rect rc;
	rc.x = xPos + (x * 6 * CHAR_SCALE);							// Work out cell position				
	rc.w = CHAR_SCALE;rc.h = CHAR_SCALE; 						// and draw sizes.
	ch = (ch & 0x7F);if (ch < 0x20) ch = '.';					// Process character
	SDL_SetRenderDrawColor(dbgRenderer,r,g,b,SDL_ALPHA_OPAQUE);	// Set colour.
	for (int x1 = 0;x1 < 5;x1++) {
		rc.y = yPos + (y * 8 * CHAR_SCALE);
		int pixData = fontdata[(ch - 0x20) * 5 + x1];
		while (pixData != 0) {
			if (pixData & 1) SDL_RenderFillRect(dbgRenderer,&rc);
			pixData = pixData >> 1;
			rc.y += rc.h;
		}
		rc.x += CHAR_SCALE;										// Horizontal spacing.
	}
}
