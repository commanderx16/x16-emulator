#ifndef _SYMBOLS_H
#define _SYMBOLS_H

#include <stdint.h>
#include "../iniparser/iniparser.h"

#define SYMBOL_LABEL_MAXLEN 14

#define SYMBOL_NOT_FOUND ((unsigned int)0xFF000000)

typedef enum { Default, User } SymFileType;
typedef enum { NoErr= 0, CantOpen= -1, SynErr= -2 } SymLoadError;

typedef struct {
	uint8_t addr8;
	char label[SYMBOL_LABEL_MAXLEN + 1];
} TSymbolDictEntry;

typedef struct {
	int count;
	TSymbolDictEntry *entries;
} TSymbolDictIndex;

typedef TSymbolDictIndex TSymbolDict[256];

typedef struct {
	int bank;
	TSymbolDict dict;
} TSymbolVolume;

void symbol_init();
void symbol_free();

TSymbolVolume *symbol_get_volume(int idx);
int symbol_add(int bank, int addr, char *label);
char *symbol_find_label(int bank, int addr);
int symbol_find_addr(int bank, char *label);
int symbol_clear_volume(int bank);
SymLoadError symbol_load(char *filename, int bank, int *addedCount, int *dupCount, int *lineCount);
void symbol_dump(char *filename);
const char *symbol_lookup(char *label);

int var_get_list(foreachCallback callback);
int var_define(const char *name, const char *info, const int *value);
int var_get(const char *name, const char **info);
int var_set(const char *name, const int value);
int var_exists(const char *name);

char *ltrim(char *s);

#endif
