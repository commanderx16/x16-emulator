#include <stdio.h>
#include <stdint.h>

void prtchflush(uint8_t c);
void prtflush(const char *s);
void prtnumflush(const char *s, uint8_t c);

void echochar(uint8_t c) {
	static int mode = 0; /* 0: PETSCII, 1: ISO8859-15 */
	static int shifted = 0; /* 0: Unshifted, 1: Shifted */
	static int color = 37;
	if ((0x00 <= c && c <= 0x1F) || (0x80 <= c && c <= 0x9F)) {
		switch (c) {
			case 0x05: prtflush("\e[37m"); color = 37; break; /* white */
			case 0x0A: prtflush("\e[0m"); color = 37; break; /* This might be compensation due to bug in emulator */
			case 0x0D: prtflush("\n\e[0m"); color = 37; break; /* This might be compensation due to bug in emulator */
			case 0x0E: shifted = 1; break;    /* lower case */
			case 0x0F: mode = 1; shifted = 1; break;
			case 0x11: prtflush("\e[B"); break; /* down */
			case 0x12: prtflush("\e[7m"); break; /* reverse on */
			case 0x13: prtflush("\e[H"); break; /* home */
			case 0x14: prtflush("\x7F"); break; /* delete */
			case 0x1C: prtflush("\e[31m"); color = 31; break; /* red */
			case 0x1D: prtflush("\e[C"); break; /* forward */
			case 0x1E: prtflush("\e[32m"); color = 32; break; /* green */
			case 0x81: prtflush("\e[33m"); color = 33; break; /* orange */
			case 0x8E: shifted = 0; break;    /* upper case */
			case 0x8F: mode = 0; shifted = 0; break;
			case 0x91: prtflush("\e[A"); break; /* up */
			case 0x92:
				prtflush("\e[0;");
				prtnumflush("%d", color);
				prtflush("m");
				break; /* reverse off */
			case 0x93: prtflush("\e[2J\e[H"); break; /* clr */
			case 0x9A: prtflush("\e[34m"); color = 34; break; /* blue */
			case 0x9C: prtflush("\e[35m"); color = 35; break; /* purple */
			case 0x9D: prtflush("\e[D"); break; /* backward */
			case 0x9E: prtflush("\e[93m"); color = 93; break; /* yellow */
			case 0x9F: prtflush("\e[36m"); color = 36; break; /* cyan */
			default: prtnumflush("\\x%02X", c);
		}
	} else {
		switch (mode) {
			case 0:
				switch (shifted) {
					case 0:
						switch (c) {
							case 0x5C: prtflush("\xC2\xA3"); break; // 0xA3, £
							case 0x5E: prtflush("\xE2\x86\x91"); break; // 0x2191, ↑
							case 0x5F: prtflush("\xE2\x86\x90"); break; // 0x2190, ←
							case 0x60: prtflush("\xE2\x94\x81"); break; // 0x2501, ━
							case 0x61: prtflush("\xE2\x99\xA0"); break; // 0x2660, ♠
							case 0x62: prtflush("\xE2\x94\x82"); break; // 0x2502, │
							case 0x63: prtflush("\xE2\x94\x81"); break; // 0x2501, ━
							case 0x69: prtflush("\xE2\x95\xAE"); break; // 0x256E, ╮
							case 0x6A: prtflush("\xE2\x95\xB0"); break; // 0x2570, ╰
							case 0x6B: prtflush("\xE2\x95\xAF"); break; // 0x256F, ╯
							case 0x6D: prtflush("\xE2\x95\xB2"); break; // 0x2572, ╲
							case 0x6E: prtflush("\xE2\x95\xB1"); break; // 0x2571, ╱
							case 0x71: prtflush("\xE2\x97\x8F"); break; // 0x25CF, ●
							case 0x73: prtflush("\xE2\x99\xA5"); break; // 0x2665, ♥
							case 0x75: prtflush("\xE2\x95\xAD"); break; // 0x256D, ╭
							case 0x76: prtflush("\xE2\x95\xB3"); break; // 0x2573, ╳
							case 0x77: prtflush("\xE2\x97\x8B"); break; // 0x25CB, ○
							case 0x78: prtflush("\xE2\x99\xA3"); break; // 0x2663, ♣
							case 0x7A: prtflush("\xE2\x99\xA6"); break; // 0x2666, ♦
							case 0x7B: prtflush("\xE2\x94\xBC"); break; // 0x253C, ┼
							case 0x7D: prtflush("\xE2\x94\x82"); break; // 0x2502, │
							case 0x7E: prtflush("\xCF\x80"); break; // 0x3C0, π
							case 0x7F: prtflush("\xE2\x97\xA5"); break; // 0x25E5, ◥
							case 0xA0: prtflush("\xC2\xA0"); break; // 0xA0,  
							case 0xA1: prtflush("\xE2\x96\x8C"); break; // 0x258C, ▌
							case 0xA2: prtflush("\xE2\x96\x84"); break; // 0x2584, ▄
							case 0xA3: prtflush("\xE2\x96\x94"); break; // 0x2594, ▔
							case 0xA4: prtflush("\xE2\x96\x81"); break; // 0x2581, ▁
							case 0xA5: prtflush("\xE2\x96\x8F"); break; // 0x258F, ▏
							case 0xA6: prtflush("\xE2\x96\x92"); break; // 0x2592, ▒
							case 0xA7: prtflush("\xE2\x96\x95"); break; // 0x2595, ▕
							case 0xA9: prtflush("\xE2\x97\xA4"); break; // 0x25E4, ◤
							case 0xAB: prtflush("\xE2\x94\x9C"); break; // 0x251C, ├
							case 0xAD: prtflush("\xE2\x94\x94"); break; // 0x2514, └
							case 0xAE: prtflush("\xE2\x94\x90"); break; // 0x2510, ┐
							case 0xAF: prtflush("\xE2\x96\x82"); break; // 0x2582, ▂
							case 0xB0: prtflush("\xE2\x94\x8C"); break; // 0x250C, ┌
							case 0xB1: prtflush("\xE2\x94\xB4"); break; // 0x2534, ┴
							case 0xB2: prtflush("\xE2\x94\xAC"); break; // 0x252C, ┬
							case 0xB3: prtflush("\xE2\x94\xA4"); break; // 0x2524, ┤
							case 0xB4: prtflush("\xE2\x96\x8E"); break; // 0x258E, ▎
							case 0xB5: prtflush("\xE2\x96\x8D"); break; // 0x258D, ▍
							case 0xB9: prtflush("\xE2\x96\x83"); break; // 0x2583, ▃
							case 0xBD: prtflush("\xE2\x94\x98"); break; // 0x2518, ┘
							case 0xC0: prtflush("\xE2\x94\x81"); break; // 0x2501, ━
							case 0xC1: prtflush("\xE2\x99\xA0"); break; // 0x2660, ♠
							case 0xC2: prtflush("\xE2\x94\x82"); break; // 0x2502, │
							case 0xC3: prtflush("\xE2\x94\x81"); break; // 0x2501, ━
							case 0xC9: prtflush("\xE2\x95\xAE"); break; // 0x256E, ╮
							case 0xCA: prtflush("\xE2\x95\xB0"); break; // 0x2570, ╰
							case 0xCB: prtflush("\xE2\x95\xAF"); break; // 0x256F, ╯
							case 0xCD: prtflush("\xE2\x95\xB2"); break; // 0x2572, ╲
							case 0xCE: prtflush("\xE2\x95\xB1"); break; // 0x2571, ╱
							case 0xD1: prtflush("\xE2\x97\x8F"); break; // 0x25CF, ●
							case 0xD3: prtflush("\xE2\x99\xA5"); break; // 0x2665, ♥
							case 0xD5: prtflush("\xE2\x95\xAD"); break; // 0x256D, ╭
							case 0xD6: prtflush("\xE2\x95\xB3"); break; // 0x2573, ╳
							case 0xD7: prtflush("\xE2\x97\x8B"); break; // 0x25CB, ○
							case 0xD8: prtflush("\xE2\x99\xA3"); break; // 0x2663, ♣
							case 0xDA: prtflush("\xE2\x99\xA6"); break; // 0x2666, ♦
							case 0xDB: prtflush("\xE2\x94\xBC"); break; // 0x253C, ┼
							case 0xDD: prtflush("\xE2\x94\x82"); break; // 0x2502, │
							case 0xDE: prtflush("\xCF\x80"); break; // 0x3C0, π
							case 0xDF: prtflush("\xE2\x97\xA5"); break; // 0x25E5, ◥
							case 0xE0: prtflush("\xC2\xA0"); break; // 0xA0,  
							case 0xE1: prtflush("\xE2\x96\x8C"); break; // 0x258C, ▌
							case 0xE2: prtflush("\xE2\x96\x84"); break; // 0x2584, ▄
							case 0xE3: prtflush("\xE2\x96\x94"); break; // 0x2594, ▔
							case 0xE4: prtflush("\xE2\x96\x81"); break; // 0x2581, ▁
							case 0xE5: prtflush("\xE2\x96\x8F"); break; // 0x258F, ▏
							case 0xE6: prtflush("\xE2\x96\x92"); break; // 0x2592, ▒
							case 0xE7: prtflush("\xE2\x96\x95"); break; // 0x2595, ▕
							case 0xE9: prtflush("\xE2\x97\xA4"); break; // 0x25E4, ◤
							case 0xEB: prtflush("\xE2\x94\x9C"); break; // 0x251C, ├
							case 0xED: prtflush("\xE2\x94\x94"); break; // 0x2514, └
							case 0xEE: prtflush("\xE2\x94\x90"); break; // 0x2510, ┐
							case 0xEF: prtflush("\xE2\x96\x82"); break; // 0x2582, ▂
							case 0xF0: prtflush("\xE2\x94\x8C"); break; // 0x250C, ┌
							case 0xF1: prtflush("\xE2\x94\xB4"); break; // 0x2534, ┴
							case 0xF2: prtflush("\xE2\x94\xAC"); break; // 0x252C, ┬
							case 0xF3: prtflush("\xE2\x94\xA4"); break; // 0x2524, ┤
							case 0xF4: prtflush("\xE2\x96\x8E"); break; // 0x258E, ▎
							case 0xF5: prtflush("\xE2\x96\x8D"); break; // 0x258D, ▍
							case 0xF9: prtflush("\xE2\x96\x83"); break; // 0x2583, ▃
							case 0xFD: prtflush("\xE2\x94\x98"); break; // 0x2518, ┘
							case 0xFF: prtflush("\xCF\x80"); break; // 0x3C0, π
							default: prtnumflush("%c", c);
						}
						break;
					case 1:
						switch (c) {
							case 0x5C: prtflush("\xC2\xA3"); break; // 0xA3, £
							case 0x5E: prtflush("\xE2\x86\x91"); break; // 0x2191, ↑
							case 0x5F: prtflush("\xE2\x86\x90"); break; // 0x2190, ←
							case 0x60: prtflush("\xE2\x94\x81"); break; // 0x2501, ━
							case 0x7B: prtflush("\xE2\x94\xBC"); break; // 0x253C, ┼
							case 0x7D: prtflush("\xE2\x94\x82"); break; // 0x2502, │
							case 0x7E: prtflush("\xE2\x96\x92"); break; // 0x2592, ▒
							case 0xA0: prtflush("\xC2\xA0"); break; // 0xA0,  
							case 0xA1: prtflush("\xE2\x96\x8C"); break; // 0x258C, ▌
							case 0xA2: prtflush("\xE2\x96\x84"); break; // 0x2584, ▄
							case 0xA3: prtflush("\xE2\x96\x94"); break; // 0x2594, ▔
							case 0xA4: prtflush("\xE2\x96\x81"); break; // 0x2581, ▁
							case 0xA5: prtflush("\xE2\x96\x8F"); break; // 0x258F, ▏
							case 0xA6: prtflush("\xE2\x96\x92"); break; // 0x2592, ▒
							case 0xA7: prtflush("\xE2\x96\x95"); break; // 0x2595, ▕
							case 0xAB: prtflush("\xE2\x94\x9C"); break; // 0x251C, ├
							case 0xAD: prtflush("\xE2\x94\x94"); break; // 0x2514, └
							case 0xAE: prtflush("\xE2\x94\x90"); break; // 0x2510, ┐
							case 0xAF: prtflush("\xE2\x96\x82"); break; // 0x2582, ▂
							case 0xB0: prtflush("\xE2\x94\x8C"); break; // 0x250C, ┌
							case 0xB1: prtflush("\xE2\x94\xB4"); break; // 0x2534, ┴
							case 0xB2: prtflush("\xE2\x94\xAC"); break; // 0x252C, ┬
							case 0xB3: prtflush("\xE2\x94\xA4"); break; // 0x2524, ┤
							case 0xB4: prtflush("\xE2\x96\x8E"); break; // 0x258E, ▎
							case 0xB5: prtflush("\xE2\x96\x8D"); break; // 0x258D, ▍
							case 0xB9: prtflush("\xE2\x96\x83"); break; // 0x2583, ▃
							case 0xBA: prtflush("\xE2\x9C\x93"); break; // 0x2713, ✓
							case 0xBD: prtflush("\xE2\x94\x98"); break; // 0x2518, ┘
							case 0xC0: prtflush("\xE2\x94\x81"); break; // 0x2501, ━
							case 0xDB: prtflush("\xE2\x94\xBC"); break; // 0x253C, ┼
							case 0xDD: prtflush("\xE2\x94\x82"); break; // 0x2502, │
							case 0xDE: prtflush("\xE2\x96\x92"); break; // 0x2592, ▒
							case 0xE0: prtflush("\xC2\xA0"); break; // 0xA0,  
							case 0xE1: prtflush("\xE2\x96\x8C"); break; // 0x258C, ▌
							case 0xE2: prtflush("\xE2\x96\x84"); break; // 0x2584, ▄
							case 0xE3: prtflush("\xE2\x96\x94"); break; // 0x2594, ▔
							case 0xE4: prtflush("\xE2\x96\x81"); break; // 0x2581, ▁
							case 0xE5: prtflush("\xE2\x96\x8F"); break; // 0x258F, ▏
							case 0xE6: prtflush("\xE2\x96\x92"); break; // 0x2592, ▒
							case 0xE7: prtflush("\xE2\x96\x95"); break; // 0x2595, ▕
							case 0xEB: prtflush("\xE2\x94\x9C"); break; // 0x251C, ├
							case 0xED: prtflush("\xE2\x94\x94"); break; // 0x2514, └
							case 0xEE: prtflush("\xE2\x94\x90"); break; // 0x2510, ┐
							case 0xEF: prtflush("\xE2\x96\x82"); break; // 0x2582, ▂
							case 0xF0: prtflush("\xE2\x94\x8C"); break; // 0x250C, ┌
							case 0xF1: prtflush("\xE2\x94\xB4"); break; // 0x2534, ┴
							case 0xF2: prtflush("\xE2\x94\xAC"); break; // 0x252C, ┬
							case 0xF3: prtflush("\xE2\x94\xA4"); break; // 0x2524, ┤
							case 0xF4: prtflush("\xE2\x96\x8E"); break; // 0x258E, ▎
							case 0xF5: prtflush("\xE2\x96\x8D"); break; // 0x258D, ▍
							case 0xF9: prtflush("\xE2\x96\x83"); break; // 0x2583, ▃
							case 0xFA: prtflush("\xE2\x9C\x93"); break; // 0x2713, ✓
							case 0xFD: prtflush("\xE2\x94\x98"); break; // 0x2518, ┘
							case 0xFF: prtflush("\xE2\x96\x92"); break; // 0x2592, ▒
							default:
								if ('A' <=  c && c <= 'Z') {
									c += 'a' - 'A';
								} else if ('a' <=  c && c <= 'z') {
									c -= 'a' - 'A';
								}
								prtnumflush("%c", c);
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

void prtchflush(uint8_t c) {
	/* ISO8859-15 */
        switch (c) {
		case 0xA4: prtflush("\xC2\xA4"); break; // 0xA4, €
		case 0xA6: prtflush("\xC2\xA6"); break; // 0xA6, Š
		case 0xA8: prtflush("\xC2\xA8"); break; // 0xA8, š
		case 0xB4: prtflush("\xC2\xB4"); break; // 0xB4, Ž
		case 0xB8: prtflush("\xC2\xB8"); break; // 0xB8, ž
		case 0xBC: prtflush("\xC2\xBC"); break; // 0xBC, Œ
		case 0xBD: prtflush("\xC2\xBD"); break; // 0xBD, œ
		case 0xBE: prtflush("\xC2\xBE"); break; // 0xBE, Ÿ
		default:
			if (c < 128) {
				putchar(c);
			} else {
				putchar(0xC2+(c > 0xBF));
				putchar((c & 0x3F)+0x80);
			}
	}
	fflush(stdout);
}
/* Test in X16 BASIC V2:
001 REM SHOW ISO8859-15 TABLE SHIFTED ON X16
010 GOTO 370
020 REM MODULO
030 MOD%=INT((X/Y-INT(X/Y))*Y+.5):RETURN
040 REM PRINT HEADING AND CHARACTER
050 X=I:Y=16:GOSUB 30
060 IF MOD%<>0 THEN GOTO 90
070 IF I<>$20 THEN ?:? "  ";V$;
080 ?:? CHR$(I/16+N);"0";V$;
090 ? CHR$(I);" ";
100 RETURN
200 REM PRINT TABLE
210 N=ASC("0")
220 B$="-":V$=CHR$($7C)
230 ?:?:?:? "  ";V$;
240 FOR I=ASC("0") TO ASC("9"):? CHR$(I);" ";:NEXT
250 FOR I=ASC("A") TO ASC("F"):? CHR$(I);" ";:NEXT
260 ?:? B$;B$;"+";:FOR I=1 TO 31:? B$;:NEXT
270 FOR I=$20 TO $7F
280 GOSUB 50
290 NEXT
300 N=ASC("7")
310 FOR I=$A0 TO $FF
320 GOSUB 50
330 NEXT
340 ? CHR$($13);
350 RETURN
360 REM MAIN
370 IF PEEK($D9)<>40 THEN SYS $FF5F
375 ? CHR$($0F);CHR$($93);
380 GOSUB 210
385 REM LOOP
390 ? CHR$($0E);CHR$($13):? "ISO8859-15   (SHIFTED)";
430 GOTO 390
*/

void prtflush(const char *s) {
	printf(s);
	fflush(stdout);
}

void prtnumflush(const char *s, uint8_t c) {
	printf(s, c);
	fflush(stdout);
}
