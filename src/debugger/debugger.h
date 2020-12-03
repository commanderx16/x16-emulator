// *******************************************************************************************
// *******************************************************************************************
//
//		File:		debugger.h
//		Date:		5th September 2019
//		Purpose:	Debugger header
//		Author:		Paul Robson (paul@robson.org.uk)
//
// *******************************************************************************************
// *******************************************************************************************

#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#include <SDL.h>

#define DDUMP_RAM	0
#define DDUMP_VERA	1

extern int showDebugOnRender;

void DEBUGRenderDisplay(int width,int height);
void DEBUGBreakToDebugger(void);
int  DEBUGGetCurrentStatus(void);
void DEBUGSetBreakPoint(int newBreakPoint);
void DEBUGInitUI(SDL_Renderer *pRenderer);
void DEBUGFreeUI();
void bootstrapConsole();

#define DBG_WIDTH 		(60)									// Char cells across
#define DBG_HEIGHT 		(55)

#define DBG_ASMX 		(1)										// Disassembly starts here
#define DBG_LBLX 		(26) 									// Debug labels start here
#define DBG_DATX		(30)									// Debug data starts here.
// #define DBG_STCK		(40)									// Debug stack starts here.
#define DBG_MEMX 		(1)										// Memory Display starts here

#define DMODE_STOP 		(0)										// Debugger is waiting for action.
#define DMODE_STEP 		(1)										// Debugger is doing a single step
#define DMODE_RUN 		(2)										// Debugger is running normally.

#endif
