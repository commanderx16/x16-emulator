#include <stdio.h>
#include <stdint.h>

#include "utf8_encode.h"

void prtchflush(uint8_t c);
void prtuptflush(uint32_t utf);
void prtflush(const char *s);
void prtnumflush(const char *s, uint8_t c);

void
echochar(uint8_t c)
{   // TODO: handle quote mode
	static int mode = 0; /* 0: PETSCII, 1: ISO8859-15 */
	static int shifted = 0; /* 0: Unshifted, 1: Shifted */
	static int color = 97;
	if ((0x00 <= c && c <= 0x1F) || (0x80 <= c && c <= 0x9F)) {
		switch (c) {
			case 0x05: prtnumflush("\e[%dm", color = 97); break; /* white */
			case 0x0A: prtuptflush(0xFFFD); /* intentional fall through */
			case 0x0D: /* CR, but acts like NL */
			case 0x8D: prtflush("\n"); /* LF, but acts like NL */
			case 0x92: prtnumflush("\e[0;%dm", color); break; /* reverse off */
			case 0x0E: shifted = 1; break;    /* lower case, text mode */
			case 0x0F: mode = 1; shifted = 1; break; /* ISO mode */
			case 0x11: prtflush("\e[B"); break; /* down */
			case 0x12: prtflush("\e[7m"); break; /* reverse on */
			case 0x13: prtflush("\e[H"); break; /* home */
			case 0x14: prtflush("\x7F"); break; /* delete, maybe change to backspace */
			case 0x1B: prtflush("\e"); break; /* escape, not part of PETSCII, but useful in VT100 */
			case 0x1C: prtnumflush("\e[%dm", color = 31); break; /* red */
			case 0x1D: prtflush("\e[C"); break; /* forward */
			case 0x1E: prtnumflush("\e[%dm", color = 32); break; /* green */
			case 0x1F: prtnumflush("\e[%dm", color = 34); break; /* blue */
			case 0x81: prtnumflush("\e[%dm", color = 33); break; /* orange */
			/*   0x8D see above */
			case 0x8E: shifted = 0; break;    /* upper case, graphics */
			case 0x8F: mode = 0; shifted = 0; break; /* PETSCII-UC mode */
			case 0x90: prtnumflush("\e[%dm", color = 30); break; /* black */
			case 0x91: prtflush("\e[A"); break; /* up */
			/*   0x92 see above */
			case 0x93: prtflush("\e[2J\e[H"); break; /* clr */
			case 0x94: prtflush("\e[1@"); break; /* insert */
			case 0x95: prtnumflush("\e[%dm", color = 33); break; /* brown, same as orange now */
			case 0x96: prtnumflush("\e[%dm", color = 91); break; /* light red */
			case 0x97: prtnumflush("\e[%dm", color = 90); break; /* dark gray */
			case 0x98: prtnumflush("\e[%dm", color = 90); break; /* middle gray, same as dark gray now */
			case 0x99: prtnumflush("\e[%dm", color = 92); break; /* light green */
			case 0x9A: prtnumflush("\e[%dm", color = 94); break; /* light blue */
			case 0x9B: prtnumflush("\e[%dm", color = 37); break; /* light gray */
			case 0x9C: prtnumflush("\e[%dm", color = 35); break; /* purple */
			case 0x9D: prtflush("\e[D"); break; /* backward */
			case 0x9E: prtnumflush("\e[%dm", color = 93); break; /* yellow */
			case 0x9F: prtnumflush("\e[%dm", color = 36); break; /* cyan */
			default: prtnumflush("\\x%02X", c);
		}
	} else {
		switch (mode) {
			case 0: /* PETSCII - when no PETSCII character exists a ISO8859-15 character is shown */
				if ('\xC0' <= c && c <= '\xDF') {
					c -= '\xC0' - '\x60';
				} else if ('\xE0' <= c && c <= '\xFE') {
					c -= '\xE0' - '\xA0';
				}
				switch (c) {
					case 0x5C: prtuptflush(0x00A3); break; // £
					case 0x5E: prtuptflush(0x2191); break; // ↑
					case 0x5F: prtuptflush(0x2190); break; // ←
					case 0x60: prtuptflush(0x2500); break; // ─ diff. triad
					case 0x7B: prtuptflush(0x253C); break; // ┼
					case 0x7D: prtuptflush(0x2502); break; // │
					case 0xA0: prtuptflush(0x00A0); break; // nbsp
					case 0xA1: prtuptflush(0x258C); break; // ▌
					case 0xA2: prtuptflush(0x2584); break; // ▄
					case 0xA3: prtuptflush(0x2594); break; // ▔
					case 0xA4: prtuptflush(0x2581); break; // ▁
					case 0xA5: prtuptflush(0x258F); break; // ▏
					case 0xA6: prtuptflush(0x2592); break; // ▒
					case 0xA7: prtuptflush(0x2595); break; // ▕
					case 0xAB: prtuptflush(0x251C); break; // ├
					case 0xAC: prtuptflush(0x2597); break; // ▗ not in triad
					case 0xAD: prtuptflush(0x2514); break; // └
					case 0xAE: prtuptflush(0x2510); break; // ┐
					case 0xAF: prtuptflush(0x2582); break; // ▂
					case 0xB0: prtuptflush(0x250C); break; // ┌
					case 0xB1: prtuptflush(0x2534); break; // ┴
					case 0xB2: prtuptflush(0x252C); break; // ┬
					case 0xB3: prtuptflush(0x2524); break; // ┤
					case 0xB4: prtuptflush(0x258E); break; // ▎
					case 0xB5: prtuptflush(0x258D); break; // ▍
					case 0xB9: prtuptflush(0x2583); break; // ▃
					case 0xBB: prtuptflush(0x2596); break; // ▖ not in triad
					case 0xBC: prtuptflush(0x259D); break; // ▝ not in triad
					case 0xBD: prtuptflush(0x2518); break; // ┘
					case 0xBE: prtuptflush(0x2598); break; // ▘ not in triad
					case 0xBF: prtuptflush(0x259A); break; // ▚ not in triad
					default:
						switch (shifted) {
							case 0:
								switch (c) {
									case 0x61: prtuptflush(0x2660); break; // ♠
									case 0x62: prtuptflush(0x2502); break; // │
									case 0x63: prtuptflush(0x2500); break; // ─ diff. triad
									case 0x69: prtuptflush(0x256E); break; // ╮
									case 0x6A: prtuptflush(0x2570); break; // ╰
									case 0x6B: prtuptflush(0x256F); break; // ╯
									case 0x6D: prtuptflush(0x2572); break; // ╲
									case 0x6E: prtuptflush(0x2571); break; // ╱
									case 0x71: prtuptflush(0x25CF); break; // ●
									case 0x73: prtuptflush(0x2665); break; // ♥
									case 0x75: prtuptflush(0x256D); break; // ╭
									case 0x76: prtuptflush(0x2573); break; // ╳
									case 0x77: prtuptflush(0x25CB); break; // ○
									case 0x78: prtuptflush(0x2663); break; // ♣
									case 0x7A: prtuptflush(0x2666); break; // ♦
									case 0x7E: prtuptflush(0x03C0); break; // π
									case 0x7F: prtuptflush(0x25E5); break; // ◥
									case 0xA9: prtuptflush(0x25E4); break; // ◤
									case 0xFF: prtuptflush(0x03C0); break; // π
									default: prtchflush(c);
								}
								break;
							case 1:
								switch (c) {
									case 0x7E: prtuptflush(0x2592); break; // ▒
									case 0x7F: prtuptflush(0x2592); break; // ▒ aprox.
									case 0xA9: prtuptflush(0x2592); break; // ▒ aprox.
									case 0xBA: prtuptflush(0x2713); break; // ✓
									case 0xFF: prtuptflush(0x2592); break; // ▒
									default:
										if ('A' <=  c && c <= 'Z') {
											c += 'a' - 'A';
										} else if ('a' <=  c && c <= 'z') {
											c -= 'a' - 'A';
										}
										prtchflush(c);
								}
								break;
						}
						break;
				}
				break;
			case 1:
				if (c == 0x7F) {
					prtflush(" "); /* In X16 \x7F does not delete. */
				} else {
					prtchflush(c);
				}
				break;
		}
	}
}

void
prtchflush(uint8_t c)
{
	/* ISO8859-15 to UTF-8 */
        switch (c) {
		case 0xA4: prtuptflush(0x20AC); break; // €
		case 0xA6: prtuptflush(0x160); break; // Š
		case 0xA8: prtuptflush(0x161); break; // š
		case 0xB4: prtuptflush(0x17D); break; // Ž
		case 0xB8: prtuptflush(0x17E); break; // ž
		case 0xBC: prtuptflush(0x152); break; // Œ
		case 0xBD: prtuptflush(0x153); break; // œ
		case 0xBE: prtuptflush(0x178); break; // Ÿ
		default:
			if (c < 0x80) {
				putchar(c);
			} else {
				putchar(0xC2+(c > 0xBF));
				putchar((c & 0x3F)+0x80);
			}
	}
	fflush(stdout);
}

void
prtuptflush(uint32_t utf)
{
	static char out[5];
	utf8_encode(out, utf);
	prtflush(out);
}

void
prtflush(const char *s)
{
	printf(s);
	fflush(stdout);
}

void
prtnumflush(const char *s, uint8_t c)
{
	printf(s, c);
	fflush(stdout);
}

/* Test program: https://github.com/mobluse/chargen-maker/blob/master/petsciiiso.bas */
