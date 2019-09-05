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

extern int showDebugOnRender;

void DEBUGRenderDisplay(int width,int height,SDL_Renderer *pRenderer);

static void DEBUGWrite(int x,int y,int ch,int r,int g,int b);
static void DEBUGString(int x,int y,char *s,int r,int g,int b);
static void DEBUGNumber(int x,int y,int n,int w,int r,int g,int b);
static void DEBUGRenderData(int y,int data);

static int DEBUGRenderRegisters(void);
static void DEBUGRenderCode(int lines,int initialPC);
#endif
