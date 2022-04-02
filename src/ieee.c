// Commander X16 Emulator
// Copyright (c) 2022 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include "rom_symbols.h"
#include "glue.h"

char name[80];
int namelen = 0;
char secaddr = 0;
bool listening = false;
bool talking = false;

char data[] = { 1, 8, 3, 8, 10, 0, 0x9e , 'M', 'S', 0, 0, 0 };
int data_ptr = 0;

void
SECOND() {
	printf("%s $%02x\n", __func__, a);
	if (listening) {
		secaddr = a;
		switch (secaddr & 0xf0) {
			case 0x60:
				printf("  WRITE %d...\n", a & 0xf);
				break;
			case 0xe0:
				printf("  CLOSE %d\n", a & 0xf);
				break;
			case 0xf0:
				printf("  OPEN %d...\n", a & 0xf);
				break;
		}
	}
}

void
TKSA() {
	printf("%s $%02x\n", __func__, a);
	if (talking) {
		secaddr = a;
	}
}

void
SETTMO() {
	printf("%s\n", __func__);
}

void
ACPTR() {
	a = data[data_ptr++];
	if (data_ptr == sizeof(data)) {
		RAM[STATUS] = 0x40;
	}
	printf("%s-> $%02x\n", __func__, a);
}

void
CIOUT() {
	printf("%s $%02x\n", __func__, a);
	if (listening) {
		if ((secaddr & 0xf0) == 0xf0) {
			if (namelen < sizeof(name)) {
				name[namelen++] = a;
			}
		} else {
			// write to file
		}
	}
}

void
UNTLK() {
	printf("%s\n", __func__);
	talking = false;
}

void
UNLSN() {
	printf("%s\n", __func__);
	listening = false;
	if ((secaddr & 0xf0) == 0xf0) {
		printf("  OPEN \"%s\",%d\n", name, secaddr & 0xf);
		data_ptr = 0;
	}
}

void
LISTEN() {
	printf("%s $%02x\n", __func__, a);
	if (a == 8) {
		listening = true;
	}
}

void
TALK() {
	printf("%s $%02x\n", __func__, a);
	if (a == 8) {
		talking = true;
	}
}
