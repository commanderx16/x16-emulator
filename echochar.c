#include <stdio.h>
#include <stdint.h>

void echochar(uint8_t c) {
	static int mode = 0; /* 0: PETSCII, 1: ISO8859-15 */
	static int shifted = 0; /* 0: Unshifted, 1: Shifted */
	if ((0x00 <= c && c <= 0x1F) || (0x80 <= c && c <= 0x9F)) {
		switch (c) {
			case 0x0D: printf("\n"); break;
			case 0x0E: shifted = 1; break;
			case 0x0F: mode = 1; shifted = 1; break;
			case 0x11: printf("\e[B"); break; /* down */
			case 0x13: printf("\e[H"); break; /* home */
			case 0x1D: printf("\e[C"); break; /* forward */
			case 0x8E: shifted = 0; break;
			case 0x8F: mode = 0; shifted = 0; break;
			case 0x91: printf("\e[A"); break; /* up */
			case 0x93: printf("\e[2J\e[H"); break; /* clr */
			case 0x9D: printf("\e[D"); break; /* backward */
			default: printf("\\x%02X", c);
		}
	} else {
		switch (mode) {
			case 0:
				switch (shifted) {
					case 0:
						switch (c) {
							case 126: printf("π"); break;
							case 127: printf("◥"); break;
							case 255: printf("π"); break;
							default: printf("%c", c);
						}
						break;
					case 1:
						switch (c) {
							case 126: printf("▒"); break;
							case 127: printf("?"); break;
							case 255: printf("▒"); break;
							default: printf("%c", c);
						}
						break;
				}
				break;
			case 1:
				switch (c) {
					case 127: printf(" "); break;
					/* ISO8859-15 */
/* Generated by X16 BASIC V2:
10 FOR I=$A0 TO $FF
20 PRINT "case";I;CHR$($9D)": printf(";CHR$(34);CHR$(I);CHR$(34);"); break;"
30 NEXT
*/
					case 160: printf(" "); break;
					case 161: printf("¡"); break;
					case 162: printf("¢"); break;
					case 163: printf("£"); break;
					case 164: printf("€"); break;
					case 165: printf("¥"); break;
					case 166: printf("Š"); break;
					case 167: printf("§"); break;
					case 168: printf("š"); break;
					case 169: printf("©"); break;
					case 170: printf("ª"); break;
					case 171: printf("«"); break;
					case 172: printf("¬"); break;
					case 173: printf("­"); break;
					case 174: printf("®"); break;
					case 175: printf("¯"); break;
					case 176: printf("°"); break;
					case 177: printf("±"); break;
					case 178: printf("²"); break;
					case 179: printf("³"); break;
					case 180: printf("Ž"); break;
					case 181: printf("µ"); break;
					case 182: printf("¶"); break;
					case 183: printf("·"); break;
					case 184: printf("ž"); break;
					case 185: printf("¹"); break;
					case 186: printf("º"); break;
					case 187: printf("»"); break;
					case 188: printf("Œ"); break;
					case 189: printf("œ"); break;
					case 190: printf("Ÿ"); break;
					case 191: printf("¿"); break;
					case 192: printf("À"); break;
					case 193: printf("Á"); break;
					case 194: printf("Â"); break;
					case 195: printf("Ã"); break;
					case 196: printf("Ä"); break;
					case 197: printf("Å"); break;
					case 198: printf("Æ"); break;
					case 199: printf("Ç"); break;
					case 200: printf("È"); break;
					case 201: printf("É"); break;
					case 202: printf("Ê"); break;
					case 203: printf("Ë"); break;
					case 204: printf("Ì"); break;
					case 205: printf("Í"); break;
					case 206: printf("Î"); break;
					case 207: printf("Ï"); break;
					case 208: printf("Ð"); break;
					case 209: printf("Ñ"); break;
					case 210: printf("Ò"); break;
					case 211: printf("Ó"); break;
					case 212: printf("Ô"); break;
					case 213: printf("Õ"); break;
					case 214: printf("Ö"); break;
					case 215: printf("×"); break;
					case 216: printf("Ø"); break;
					case 217: printf("Ù"); break;
					case 218: printf("Ú"); break;
					case 219: printf("Û"); break;
					case 220: printf("Ü"); break;
					case 221: printf("Ý"); break;
					case 222: printf("Þ"); break;
					case 223: printf("ß"); break;
					case 224: printf("à"); break;
					case 225: printf("á"); break;
					case 226: printf("â"); break;
					case 227: printf("ã"); break;
					case 228: printf("ä"); break;
					case 229: printf("å"); break;
					case 230: printf("æ"); break;
					case 231: printf("ç"); break;
					case 232: printf("è"); break;
					case 233: printf("é"); break;
					case 234: printf("ê"); break;
					case 235: printf("ë"); break;
					case 236: printf("ì"); break;
					case 237: printf("í"); break;
					case 238: printf("î"); break;
					case 239: printf("ï"); break;
					case 240: printf("ð"); break;
					case 241: printf("ñ"); break;
					case 242: printf("ò"); break;
					case 243: printf("ó"); break;
					case 244: printf("ô"); break;
					case 245: printf("õ"); break;
					case 246: printf("ö"); break;
					case 247: printf("÷"); break;
					case 248: printf("ø"); break;
					case 249: printf("ù"); break;
					case 250: printf("ú"); break;
					case 251: printf("û"); break;
					case 252: printf("ü"); break;
					case 253: printf("ý"); break;
					case 254: printf("þ"); break;
					case 255: printf("ÿ"); break;
					default: printf("%c", c);
				}
				break;
		}
	}
}
