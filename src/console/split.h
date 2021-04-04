/***************************************************************************
                          split.h  -  description
                             -------------------
    begin                : Mon May 27 2002
    copyright            : (C) 2002 by Clemens Wacha
    email                : wacha@gmx.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


/*! file
    Simple String Tokenizer
	
	example:
	
	input: "hello this is a "very cool" test.     "
	-> splitline(argv, max_tokens_to_return, string);
	
	output: return value of splitline = 6
			argv[0] = "hello"
			argv[1] = "this"
			argv[2] = "is"
			argv[3] = "a"
			argv[4] = "very cool"
			argv[5] = "test."  <-- remark the missing spaces
 
    @author Clemens Wacha
*/

#ifndef _SPLIT_H
#define _SPLIT_H

/*! internal function that is used by splitline() */
char *splitnext(char **pos);

/*!
	splitline is a destructive argument parser, much like a very primitive
	form of a shell parser. it supports quotes for embedded spaces and
	literal quotes with the backslash escape.
 
	splitline converts a string into 'tokens'. A 'token' is a string of characters
	not containing the space ' ' character.

	*** Remark the the original string gets cut apart! You cannot use it anymore! ***
*/
int splitline(char **argv, int max, char *line);

#endif  /* _SPLIT_H */
