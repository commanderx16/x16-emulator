#include <stdio.h>
#include <stdint.h>

void echochar(uint8_t c) {
	switch (c) {
		case 13: printf("\n"); break;
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
}
