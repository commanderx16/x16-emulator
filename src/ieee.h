// Commander X16 Emulator
// Copyright (c) 2022 Michael Steil
// All rights reserved. License: 2-clause BSD

void ieee_init();
void SECOND(uint8_t a);
void TKSA(uint8_t a);
int ACPTR(uint8_t *a);
int CIOUT(uint8_t a);
void UNTLK(void);
int UNLSN(void);
void LISTEN(uint8_t a);
void TALK(uint8_t a);
int MACPTR(uint16_t addr, uint16_t *count);
