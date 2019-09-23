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
			case 0x13: printf("\e[H"); break;
			case 0x8E: shifted = 0; break;
			case 0x8F: mode = 0; shifted = 0; break;
			case 0x93: printf("\e[2J\e[H"); break;
			default: printf("%c", c);
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
					case 163: printf("£"); break;
					case 164: printf("€"); break;
					case 167: printf("§"); break;
					case 172: printf("¬"); break;
					case 181: printf("µ"); break;
					case 196: printf("Ä"); break;
					case 197: printf("Å"); break;
					case 214: printf("Ö"); break;
					case 228: printf("ä"); break;
					case 229: printf("å"); break;
					case 246: printf("ö"); break;
					default: printf("%c", c);
				}
				break;
		}
	}
}
