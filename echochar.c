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
							case 126: prtflush("π"); break;
							case 127: prtflush("◥"); break;
							case 0xA9: prtflush("◤"); break;
							case 0xDF: prtflush("◥"); break;
							case 0xE9: prtflush("◤"); break;
							case 255: prtflush("π"); break;
							default: prtnumflush("%c", c);
						}
						break;
					case 1:
						switch (c) {
							case 126: prtflush("▒"); break;
							case 127: prtflush("?"); break;
							case 0xDF: prtflush("?"); break;
							case 255: prtflush("▒"); break;
							default: prtnumflush("%c", c);
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
		case 0xA4: prtflush("€"); break;
		case 0xA6: prtflush("Š"); break;
		case 0xA8: prtflush("š"); break;
		case 0xB4: prtflush("Ž"); break;
		case 0xB8: prtflush("ž"); break;
		case 0xBC: prtflush("Œ"); break;
		case 0xBD: prtflush("œ"); break;
		case 0xBE: prtflush("Ÿ"); break;
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
