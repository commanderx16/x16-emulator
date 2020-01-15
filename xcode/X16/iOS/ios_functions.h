//
//  ios_functions.h
//  CommanderX16
//
//  ; (C)2020 Matthew Pearce, License: 2-clause BSD
//

#ifndef ios_functions_h
#define ios_functions_h

#include <stdio.h>
#include "rom_symbols.h"
#include "utf8.h"
#include "glue.h"

void sendNotification(const char *notification_name);
void createIosMessageObserver(void);
void receiveFile(char* dropped_filedir);
int loadPrgFile(const char *prg_path);
int loadBasFile(const char *bas_path);

//patch required for SDL
void memset_pattern4(void *__b, const void *__pattern4, size_t __len);

uint8_t iso8859_15_from_unicode(uint32_t c);
uint32_t unicode_from_iso8859_15(uint8_t c);
static void print_iso8859_15_char(char c);

#endif /* ios_functions_h */
