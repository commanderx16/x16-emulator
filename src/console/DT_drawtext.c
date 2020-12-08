/*
	SDL_console: An easy to use drop-down console based on the SDL library
	Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Clemens Wacha

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WHITOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU Library Generla Public
	License along with this library; if not, write to the Free
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	Clemens Wacha
	reflex-2000@gmx.net
*/


/*  DT_drawtext.c
 *  Written By: Garrett Banuk <mongoose@mongeese.org>
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"
#include "DT_drawtext.h"
#include "internal.h"
#include <libgen.h>

#ifdef HAVE_SDLIMAGE
  #include "SDL_image.h"
#endif

#define CHARS_PER_LINE 16
#define CHAR_LINES 16

static BitFont *BitFonts = NULL;	/* Linked list of fonts */

/*
	src: https://misc.flogisoft.com/bash/tip_colors_and_formatting
*/
const char *DT_color_default= "\e[39m";
const char *DT_color_black= "\e[30m";
const char *DT_color_red= "\e[31m";
const char *DT_color_green= "\e[32m";
const char *DT_color_yellow= "\e[33m";
const char *DT_color_blue= "\e[34m";
const char *DT_color_white= "\e[97m";

const SDL_Color col_default= {255, 255, 255, 255};
const SDL_Color col_black= {0, 0, 0, 255};
const SDL_Color col_red= {232, 0, 0, 255};
const SDL_Color col_green= {4, 141, 4, 255};
const SDL_Color col_yellow= {205, 205, 0, 255};
const SDL_Color col_blue= {30, 144, 255, 255};
const SDL_Color col_white= {255, 255, 255, 255};

/* sets the transparency value for the font in question.  assumes that
 * we're in an OpenGL context.  */
void DT_SetFontAlphaGL(int fontNumber, int a) {
	unsigned char val;
	BitFont *CurrentFont;
	unsigned char r_targ, g_targ, b_targ;
	int i, imax;
	unsigned char *pix;

	/* get pointer to font */
	CurrentFont = DT_FontPointer(fontNumber);
	if(CurrentFont == NULL) {
		PRINT_ERROR("Setting font alpha for non-existent font!\n");
		return;
	}
	if(CurrentFont->fontSurface->format->BytesPerPixel == 2) {
		PRINT_ERROR("16-bit SDL surfaces do not support alpha-blending under OpenGL\n");
		return;
	}
	if(a < SDL_ALPHA_TRANSPARENT)
		val = SDL_ALPHA_TRANSPARENT;
	else if(a > SDL_ALPHA_OPAQUE)
		val = SDL_ALPHA_OPAQUE;
	else
		val = a;

	/* iterate over all pixels in the font surface.  For each
	 * pixel that is (255,0,255), set its alpha channel
	 * appropriately.  */
	imax = CurrentFont->fontSurface->h * (CurrentFont->fontSurface->w << 2);
	pix = (unsigned char *)(CurrentFont->fontSurface->pixels);
	r_targ = 255;		/*pix[0]; */
	g_targ = 0;		/*pix[1]; */
	b_targ = 255;		/*pix[2]; */
	for(i = 3; i < imax; i += 4)
		if(pix[i - 3] == r_targ && pix[i - 2] == g_targ && pix[i - 1] == b_targ)
			pix[i] = val;
	/* also make sure that alpha blending is disabled for the font
	   surface. */
	//SDL_SetAlpha(CurrentFont->fontSurface, 0, SDL_ALPHA_OPAQUE);
	SDL_SetSurfaceBlendMode(CurrentFont->fontSurface, SDL_BLENDMODE_NONE);
}

/* Loads the font into a new struct
 * returns -1 as an error else it returns the number
 * of the font for the user to use
 */
int DT_LoadFont(SDL_Renderer *renderer, const char *bitmapPath, int flags) {
	int fontNumber;
	BitFont **CurrentFont = &BitFonts;
	SDL_Surface *Temp;
	char fontName[32];

	// tried with basename() but it was crashing :/
	const char *filename= strrchr (bitmapPath, '/');;
	filename= filename ? filename+1 : bitmapPath;
	const char *dot= strrchr(filename, '.');
	const int len= dot ? dot - filename : strlen(filename);
	memset(fontName, 0, sizeof(fontName));
	strncpy(fontName, filename, len<sizeof(fontName)-1 ? len : sizeof(fontName)-1);

	fontNumber= DT_FindFontID(fontName);
	if(fontNumber>=0)
		return fontNumber;

	/* load the font bitmap */
	SDL_RWops * rw = SDL_RWFromFile(bitmapPath, "rb");
	if(rw == NULL)
		return -1;

#ifdef HAVE_SDLIMAGE
	Temp = IMG_Load_RW(rw, 0);
#else
	Temp = SDL_LoadBMP_RW(rw, 0);
#endif

	SDL_RWclose(rw);

    if(Temp == NULL) {
		PRINT_ERROR("Cannot load file: ");
		printf("%s\n", SDL_GetError());
		return -1;
	}

	fontNumber= 0;
	while(*CurrentFont) {
		CurrentFont = &((*CurrentFont)->nextFont);
		fontNumber++;
	}

	/* Add a font to the list */
	*CurrentFont = (BitFont *) calloc(1, sizeof(BitFont));

	(*CurrentFont)->fontSurface = SDL_ConvertSurfaceFormat(Temp, SDL_PIXELFORMAT_RGBA32, 0);

	// (*CurrentFont)->charWidth = (*CurrentFont)->fontSurface->w / 256;
	// (*CurrentFont)->charHeight = (*CurrentFont)->fontSurface->h;
	(*CurrentFont)->charWidth = (*CurrentFont)->fontSurface->w / CHARS_PER_LINE;
	(*CurrentFont)->charHeight = (*CurrentFont)->fontSurface->h / CHAR_LINES;
	(*CurrentFont)->fontNumber = fontNumber;
	(*CurrentFont)->nextFont = NULL;

	strcpy((*CurrentFont)->fontName, fontName);


	/* Set font as transparent if the flag is set.  The assumption we'll go on
	 * is that the first pixel of the font image will be the color we should treat
	 * as transparent.
	 */
	if(flags & TRANS_FONT)
		SDL_SetColorKey((*CurrentFont)->fontSurface, SDL_TRUE, SDL_MapRGB((*CurrentFont)->fontSurface->format, 255, 0, 255));

	(*CurrentFont)->fontTexture= SDL_CreateTextureFromSurface(renderer, (*CurrentFont)->fontSurface);

	return fontNumber;
}

int DT_ProcessEscapeCodes(const char *string, SDL_Surface *surface, int pos, int len) {
	int current;
	int code= 0;
	SDL_Colour col;

	if(pos >= len || string[pos] != '[')
		return pos;

	while(++pos < len) {
		current = string[pos];
		if(current == 'm') {
			switch(code) {
				case 39:	col= col_default; break;
				case 30:	col= col_black; break;
				case 31:	col= col_red; break;
				case 32:	col= col_green; break;
				case 33:	col= col_yellow; break;
				case 34:	col= col_blue; break;
				case 97:	col= col_white; break;
				default:
					col= col_default;
			}
			SDL_SetSurfaceColorMod(surface, col.r, col.g, col.b);
			break;
		}
		if(current >= '0' && current <= '9') {
			code= code * 10 + current - '0';
		}
	}
	return pos;
}

/* Takes the font type, coords, and text to draw to the surface*/
void DT_DrawText(const char *string, SDL_Surface *surface, int FontType, int x, int y) {
	int loop;
	int characters;
	int current;
	SDL_Rect SourceRect, DestRect;
	BitFont *CurrentFont;

	CurrentFont = DT_FontPointer(FontType);

	/* see how many characters can fit on the screen */
	if(x > surface->w || y > surface->h)
		return;

	if(strlen(string) < (surface->w - x) / CurrentFont->charWidth)
		characters = strlen(string);
	else
		characters = (surface->w - x) / CurrentFont->charWidth;

	DestRect.x = x;
	DestRect.y = y;
	DestRect.w = CurrentFont->charWidth;
	DestRect.h = CurrentFont->charHeight;

	SourceRect.y = 0;
	SourceRect.w = CurrentFont->charWidth;
	SourceRect.h = CurrentFont->charHeight;

	/* Now draw it */
	for(loop = 0; loop < characters; loop++) {
		current = string[loop];
		if (current<0 || current > 255)
			current = 0;

		if(current == 0x1B) { // ESC [
			loop= DT_ProcessEscapeCodes(string, CurrentFont->fontSurface, ++loop, characters);
			continue;
		}

		/* SourceRect.x = string[loop] * CurrentFont->charWidth; */
		SourceRect.x = current * CurrentFont->charWidth % (CurrentFont->charWidth * CHARS_PER_LINE);
		SourceRect.y = current / CHARS_PER_LINE * (CurrentFont->charHeight);

		SDL_BlitSurface(CurrentFont->fontSurface, &SourceRect, surface, &DestRect);
		DestRect.x += CurrentFont->charWidth;
	}
}

int DT_ProcessEscapeCodes2(const char *string, SDL_Texture *texture, int pos, int len) {
	int current;
	int code= 0;
	SDL_Colour col;

	if(pos >= len || string[pos] != '[')
		return pos;

	while(++pos < len) {
		current = string[pos];
		if(current == 'm') {
			switch(code) {
				case 39:	col= col_default; break;
				case 30:	col= col_black; break;
				case 31:	col= col_red; break;
				case 32:	col= col_green; break;
				case 33:	col= col_yellow; break;
				case 34:	col= col_blue; break;
				case 97:	col= col_white; break;
				default:
					col= col_default;
			}
			SDL_SetTextureColorMod(texture, col.r, col.g, col.b);
			break;
		}
		if(current >= '0' && current <= '9') {
			code= code * 10 + current - '0';
		}
	}
	return pos;
}

void DT_DrawText2(SDL_Renderer *renderer, const char *string, int FontType, int x, int y, SDL_Color colour) {
	int loop;
	int characters;
	int current;
	SDL_Rect SourceRect, DestRect;
	BitFont *CurrentFont;
	int width, height;

	SDL_GetRendererOutputSize(renderer, &width, &height);
	/* see how many characters can fit on the screen */
	if(x > width || y > height)
		return;

	CurrentFont = DT_FontPointer(FontType);

	SDL_SetTextureColorMod(CurrentFont->fontTexture, colour.r, colour.g, colour.b);

	if(strlen(string) < (width - x) / CurrentFont->charWidth)
		characters = strlen(string);
	else
		characters = (width - x) / CurrentFont->charWidth;

	DestRect.x = x;
	DestRect.y = y;
	DestRect.w = CurrentFont->charWidth;
	DestRect.h = CurrentFont->charHeight;

	SourceRect.y = 0;
	SourceRect.w = CurrentFont->charWidth;
	SourceRect.h = CurrentFont->charHeight;

	/* Now draw it */
	for(loop = 0; loop < characters; loop++) {
		current = string[loop];
		if (current<0 || current > 255)
			current = 0;

		if(current == 0x1B) { // ESC [
			loop= DT_ProcessEscapeCodes2(string, CurrentFont->fontTexture, ++loop, characters);
			continue;
		}

		// SourceRect.x = current * CurrentFont->charWidth;
		SourceRect.x = current * CurrentFont->charWidth % (CurrentFont->charWidth * CHARS_PER_LINE);
		SourceRect.y = current / CHARS_PER_LINE * (CurrentFont->charHeight);
		SDL_RenderCopy(renderer, CurrentFont->fontTexture, &SourceRect, &DestRect);

		DestRect.x += CurrentFont->charWidth;
	}
}


/* Returns the height of the font numbers character
 * returns 0 if the fontnumber was invalid */
int DT_FontHeight(int fontNumber) {
	BitFont *CurrentFont;

	CurrentFont = DT_FontPointer(fontNumber);
	if(CurrentFont)
		return CurrentFont->charHeight;
	else
		return 0;
}

/* Returns the width of the font numbers charcter */
int DT_FontWidth(int fontNumber) {
	BitFont *CurrentFont;

	CurrentFont = DT_FontPointer(fontNumber);
	if(CurrentFont)
		return CurrentFont->charWidth;
	else
		return 0;
}

/* Returns a pointer to the font struct of the number
 * returns NULL if there's an error
 */
BitFont *DT_FontPointer(int fontNumber) {
	BitFont *CurrentFont = BitFonts;
	// BitFont *temp;

	while(CurrentFont)
		if(CurrentFont->fontNumber == fontNumber)
			return CurrentFont;
		else {
			// temp = CurrentFont;
			CurrentFont = CurrentFont->nextFont;
		}

	return NULL;

}

/* Find a font by name or number and returns a pointer to the font struct
 * returns -1 if not found
 */
int DT_FindFontID(char *font) {
	BitFont *currentFont = BitFonts;
	char *endPtr;
	int fontNum= (int)strtol(font, &endPtr, 10);

	// not a number therefore let's find by name
	if(endPtr == font) {
		while(currentFont)
			if(!stricmp(font, currentFont->fontName))
				return currentFont->fontNumber;
			else {
				currentFont = currentFont->nextFont;
			}
	}
	else
		return DT_FontPointer(fontNum) ? fontNum : -1;

	return -1;

}

/* Set Font Name
 *
 */
int DT_SetFontName(int fontNumber, const char *name) {
	BitFont *currentFont= DT_FontPointer(fontNumber);
	if(!currentFont)
		return -1;

	int len= strlen(name);
	memset(currentFont->fontName, 0, sizeof(currentFont->fontName));
	strncpy(currentFont->fontName, name, len<sizeof(currentFont->fontName) ? len : sizeof(currentFont->fontName)-1);

	return 0;
}

/* removes all the fonts currently loaded */
void DT_DestroyDrawText() {
	BitFont *CurrentFont = BitFonts;
	BitFont *temp;

	while(CurrentFont) {
		temp = CurrentFont;
		CurrentFont = CurrentFont->nextFont;

		SDL_FreeSurface(temp->fontSurface);
		free(temp);
	}

	BitFonts = NULL;
}
