//
//  ios_functions.h
//  CommanderX16
//
//  Created by Pearce, Matthew (Senior Developer) on 04/01/2020.
//

#ifndef ios_functions_h
#define ios_functions_h

#include <stdio.h>
#include "memory.h"
#include "rom_symbols.h"
#include "utf8.h"


void sendNotification(const char *notification_name);
void createIosMessageObserver(void);
int loadPrgFile(const char *prg_path);
int loadBasFile(const char *bas_path);

uint8_t iso8859_15_from_unicode(uint32_t c);
uint32_t unicode_from_iso8859_15(uint8_t c);
static void print_iso8859_15_char(char c);

#endif /* ios_functions_h */
