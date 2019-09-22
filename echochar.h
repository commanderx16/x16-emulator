/* Prints characters when using option -echo with the correct looking characters.
   Currently only works in ISO8859-15 mode and not PETSCII and only supports characters on
   EN-GB and SV-SE keyboard.
*/

/* Commander X16 BASIC V2 program to generate ASCII/ISO8859-15 codes by pressing keys:
10 GET A$:IF A$="" THEN GOTO 10
20 PRINT A$;" =";ASC(A$)
30 GOTO 10
*/

#ifndef _ECHOCHAR_H
#define _ECHOCHAR_H

#include <stdint.h>

void echochar(uint8_t c);

#endif
