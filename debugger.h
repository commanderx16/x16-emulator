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

extern int showDebugOnRender;

void DEBUGRenderDisplay(int width,int height,SDL_Renderer *pRenderer);
void DEBUGBreakToDebugger(void);
int  DEBUGGetCurrentStatus(void);
void DEBUGSetBreakPoint(int newBreakPoint);

#define DBG_WIDTH 		(40)									// Char cells across
#define DBG_HEIGHT 		(24)

#define DBG_ASMX 		(4)										// Disassembly starts here
#define DBG_LBLX 		(26) 									// Debug labels start here
#define DBG_DATX		(30)									// Debug data starts here.
#define DBG_MEMX 		(1)										// Memory Display starts here

#define CHAR_SCALE 		(2)										// Debugger character pixel size.

#define COL_LABEL 		0,255,0									// RGB colours
#define COL_DATA 		0,255,255
#define COL_HIGHLIGHT 	255,255,0

#define DMODE_STOP 		(0)										// Debugger is waiting for action.
#define DMODE_STEP 		(1)										// Debugger is doing a single step
#define DMODE_RUN 		(2)										// Debugger is running normally.

#endif
