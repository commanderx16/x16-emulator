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

#ifdef HAVE_SDLIMAGE
  #include "SDL_image.h"
#endif


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
void DT_SetFontAlphaGL(int FontNumber, int a) {
	unsigned char val;
	BitFont *CurrentFont;
	unsigned char r_targ, g_targ, b_targ;
	int i, imax;
	unsigned char *pix;

	/* get pointer to font */
	CurrentFont = DT_FontPointer(FontNumber);
	if(CurrentFont == NULL) {
		PRINT_ERROR("Setting font alpha for non-existent font!\n");
		return;
	}
	if(CurrentFont->FontSurface->format->BytesPerPixel == 2) {
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
	imax = CurrentFont->FontSurface->h * (CurrentFont->FontSurface->w << 2);
	pix = (unsigned char *)(CurrentFont->FontSurface->pixels);
	r_targ = 255;		/*pix[0]; */
	g_targ = 0;		/*pix[1]; */
	b_targ = 255;		/*pix[2]; */
	for(i = 3; i < imax; i += 4)
		if(pix[i - 3] == r_targ && pix[i - 2] == g_targ && pix[i - 1] == b_targ)
			pix[i] = val;
	/* also make sure that alpha blending is disabled for the font
	   surface. */
	//SDL_SetAlpha(CurrentFont->FontSurface, 0, SDL_ALPHA_OPAQUE);
	SDL_SetSurfaceBlendMode(CurrentFont->FontSurface, SDL_BLENDMODE_NONE);
}

/* Loads the font into a new struct
 * returns -1 as an error else it returns the number
 * of the font for the user to use
 */
int DT_LoadFont(SDL_Renderer *renderer, const char *BitmapName, int flags) {
	int ret = -1;

	printf("DT_LoadFont %s\n", BitmapName);

	SDL_RWops * rw = SDL_RWFromFile(BitmapName, "rb");
	if (rw) {
		ret = DT_LoadFont_RW(renderer, rw, flags);
		SDL_RWclose(rw);
	}
	return ret;
}

int DT_LoadFont_RW(SDL_Renderer *renderer, SDL_RWops * rw, int flags) {
	int FontNumber = 0;
	BitFont **CurrentFont = &BitFonts;
	SDL_Surface *Temp;


	while(*CurrentFont) {
		CurrentFont = &((*CurrentFont)->NextFont);
		FontNumber++;
	}

	/* load the font bitmap */

#ifdef HAVE_SDLIMAGE
	Temp = IMG_Load_RW(rw, 0);
#else
	Temp = SDL_LoadBMP_RW(rw, 0);
#endif

    if(Temp == NULL) {
		PRINT_ERROR("Cannot load file: ");
		printf("%s\n", SDL_GetError());
		return -1;
	}

	/* Add a font to the list */
	*CurrentFont = (BitFont *) malloc(sizeof(BitFont));

	(*CurrentFont)->FontSurface = SDL_ConvertSurfaceFormat(Temp, SDL_PIXELFORMAT_RGBA32, 0);

	(*CurrentFont)->CharWidth = (*CurrentFont)->FontSurface->w / 256;
	(*CurrentFont)->CharHeight = (*CurrentFont)->FontSurface->h;
	(*CurrentFont)->FontNumber = FontNumber;
	(*CurrentFont)->NextFont = NULL;


	/* Set font as transparent if the flag is set.  The assumption we'll go on
	 * is that the first pixel of the font image will be the color we should treat
	 * as transparent.
	 */
	if(flags & TRANS_FONT)
		SDL_SetColorKey((*CurrentFont)->FontSurface, SDL_TRUE, SDL_MapRGB((*CurrentFont)->FontSurface->format, 255, 0, 255));

	(*CurrentFont)->FontTexture= SDL_CreateTextureFromSurface(renderer, (*CurrentFont)->FontSurface);

	return FontNumber;
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

	if(strlen(string) < (surface->w - x) / CurrentFont->CharWidth)
		characters = strlen(string);
	else
		characters = (surface->w - x) / CurrentFont->CharWidth;

	DestRect.x = x;
	DestRect.y = y;
	DestRect.w = CurrentFont->CharWidth;
	DestRect.h = CurrentFont->CharHeight;

	SourceRect.y = 0;
	SourceRect.w = CurrentFont->CharWidth;
	SourceRect.h = CurrentFont->CharHeight;

	/* Now draw it */
	for(loop = 0; loop < characters; loop++) {
		current = string[loop];
		if (current<0 || current > 255)
			current = 0;

		if(current == 0x1B) { // ESC [
			loop= DT_ProcessEscapeCodes(string, CurrentFont->FontSurface, ++loop, characters);
			continue;
		}

		/* SourceRect.x = string[loop] * CurrentFont->CharWidth; */
		SourceRect.x = current * CurrentFont->CharWidth;
		SDL_BlitSurface(CurrentFont->FontSurface, &SourceRect, surface, &DestRect);
		DestRect.x += CurrentFont->CharWidth;
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

	SDL_SetTextureColorMod(CurrentFont->FontTexture, colour.r, colour.g, colour.b);

	if(strlen(string) < (width - x) / CurrentFont->CharWidth)
		characters = strlen(string);
	else
		characters = (width - x) / CurrentFont->CharWidth;

	DestRect.x = x;
	DestRect.y = y;
	DestRect.w = CurrentFont->CharWidth;
	DestRect.h = CurrentFont->CharHeight;

	SourceRect.y = 0;
	SourceRect.w = CurrentFont->CharWidth;
	SourceRect.h = CurrentFont->CharHeight;

	/* Now draw it */
	for(loop = 0; loop < characters; loop++) {
		current = string[loop];
		if (current<0 || current > 255)
			current = 0;

		if(current == 0x1B) { // ESC [
			loop= DT_ProcessEscapeCodes2(string, CurrentFont->FontTexture, ++loop, characters);
			continue;
		}

		/* SourceRect.x = string[loop] * CurrentFont->CharWidth; */
		SourceRect.x = current * CurrentFont->CharWidth;
		SDL_RenderCopy(renderer, CurrentFont->FontTexture, &SourceRect, &DestRect);

		DestRect.x += CurrentFont->CharWidth;
	}
}


/* Returns the height of the font numbers character
 * returns 0 if the fontnumber was invalid */
int DT_FontHeight(int FontNumber) {
	BitFont *CurrentFont;

	CurrentFont = DT_FontPointer(FontNumber);
	if(CurrentFont)
		return CurrentFont->CharHeight;
	else
		return 0;
}

/* Returns the width of the font numbers charcter */
int DT_FontWidth(int FontNumber) {
	BitFont *CurrentFont;

	CurrentFont = DT_FontPointer(FontNumber);
	if(CurrentFont)
		return CurrentFont->CharWidth;
	else
		return 0;
}

/* Returns a pointer to the font struct of the number
 * returns NULL if theres an error
 */
BitFont *DT_FontPointer(int FontNumber) {
	BitFont *CurrentFont = BitFonts;
	// BitFont *temp;

	while(CurrentFont)
		if(CurrentFont->FontNumber == FontNumber)
			return CurrentFont;
		else {
			// temp = CurrentFont;
			CurrentFont = CurrentFont->NextFont;
		}

	return NULL;

}

/* removes all the fonts currently loaded */
void DT_DestroyDrawText() {
	BitFont *CurrentFont = BitFonts;
	BitFont *temp;

	while(CurrentFont) {
		temp = CurrentFont;
		CurrentFont = CurrentFont->NextFont;

		SDL_FreeSurface(temp->FontSurface);
		free(temp);
	}

	BitFonts = NULL;
}
