#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../iniparser/dictionary.h"
#include "symbols.h"

static TSymbolVolume *volumes= NULL;
static int volumesCount= 0;
static dictionary * symbolsDict= NULL;

#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 16
#endif

void hexdump(void *mem, unsigned int len)
{
        unsigned int i, j;

        for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
        {
                /* print offset */
                if(i % HEXDUMP_COLS == 0)
                {
                        printf("0x%06x: ", i);
                }

                /* print hex data */
                if(i < len)
                {
                        printf("%02x ", 0xFF & ((char*)mem)[i]);
                }
                else /* end of block, just aligning for ASCII dump */
                {
                        printf("   ");
                }

                /* print ASCII dump */
                if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
                {
                        for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
                        {
                                if(j >= len) /* end of block, not really printing */
                                {
                                        putchar(' ');
                                }
                                else if(isprint(((char*)mem)[j])) /* printable char */
                                {
                                        putchar(0xFF & ((char*)mem)[j]);
                                }
                                else /* other char */
                                {
                                        putchar('.');
                                }
                        }
                        putchar('\n');
                }
        }
}

// *******************************************************************************************
// left trim string
//
char *ltrim(char *s)
{
	while(isspace(*s)) s++;
	return s;
}

/*
	VARS management
*/

long long int var_getPtr(const char *name, const char **info) {
	char key[64];
	snprintf(key, 64, "var:%s", name);
	const char *value= iniparser_getstring(symbolsDict, key, 0);
	if(!value)
		return -1;
	if(info)
		*info= strchr(value, '|') + 1;
	return strtol(value, NULL, 0);
}

int var_get_list(foreachCallback callback) {
	return iniparser_foreachkeys(symbolsDict, "var", callback);
}

int var_define(const char *name, const char *info, const int *value) {
	char key[64];
	char valStr[64];
	snprintf(key, sizeof key, "var:%s", name);
	snprintf(valStr, sizeof valStr, "%lld|%s", (long long int)value, info);
	return iniparser_set(symbolsDict, key, valStr);
}

int var_get(const char *name, const char **info) {
	long long int valPtr= var_getPtr(name, info);
	return valPtr ? *((int *)valPtr) : -1;
}

int var_set(const char *name, const int value) {
	long long int valPtr= var_getPtr(name, NULL);
	if(valPtr) {
		*((int *)valPtr)= value;
	}
	return valPtr ? value : -1;
}

int var_exists(const char *name) {
	char key[64];
	snprintf(key, 64, "var:%s", name);
	return NULL != iniparser_getstring(symbolsDict, key, NULL);
}
/*
	return addresses for symbol "label"
*/
const char *symbolDict_get_addresses(char *label) {
	char key[1024];
	snprintf(key, 1024, "symbol:%s", label);
	return iniparser_getstring(symbolsDict, key, NULL);
}

/*
	set addresses for symbol "label"
*/
int symbolDict_set_addresses(char *label, char *addresses) {
	char key[1024];
	snprintf(key, 1024, "symbol:%s", label);
	return iniparser_set(symbolsDict, key, addresses);
}

/*
	expand volumes array with one new volume
*/
TSymbolVolume *symbol_add_volume(int bank) {

	volumes= volumesCount == 0 ?
				malloc(sizeof(TSymbolVolume))
				:
				realloc(volumes, (volumesCount+1) * sizeof(TSymbolVolume));

	if(volumes == NULL)
		return NULL;

	TSymbolVolume *volume= &volumes[volumesCount++];
	memset(volume, 0, sizeof(TSymbolVolume));
	volume->bank= bank;
	return volume;
}

/*
	return the volume[idx] or NULL if none
*/
TSymbolVolume *symbol_get_volume(int idx) {

	if(!volumesCount || idx >= volumesCount)
		return NULL;

	return &volumes[idx];
}

/*
	lookup for a volume for the bank and return its index if found
*/
int symbol_get_bank_volume_idx(int bank) {

	if(!volumesCount)
		return -1;

	int shelfIdx= -1;
	for(int idx= 0; idx < volumesCount; idx++) {
		if(bank == volumes[idx].bank)
			shelfIdx= idx;
	}
	return shelfIdx;
}

/*
	lookup for a volume for the bank and return it if found
*/
TSymbolVolume *symbol_get_bank_volume(int bank) {
	int shelfIdx= symbol_get_bank_volume_idx(bank);
	return shelfIdx < 0 ? NULL : &volumes[shelfIdx];
}

/*
	iterate on all entries for a dict page to match addr
*/
TSymbolDictEntry *symbol_dict_find_entry_by_addr(TSymbolVolume *dict, int addr) {
	int pageIdx= addr >> 8;
	TSymbolDictEntry *entries= dict->dict[pageIdx].entries;
	int count= dict->dict[pageIdx].count;

	addr= addr & 0xFF;
	for(int idx= 0; idx < count; idx++) {
		if(entries[idx].addr8 == addr)
			return &entries[idx];
	}

	return NULL;
}

/*
	lookup in bank dict for addr and return label
*/
char *symbol_find_label(int bank, int addr) {
	TSymbolVolume *volume= symbol_get_bank_volume(bank);
	if(volume == NULL)
		return NULL;

	TSymbolDictEntry *entry= symbol_dict_find_entry_by_addr(volume, addr);
	if(entry == NULL)
		return NULL;

	return entry->label;
}

/*
	iterate on all entries for a dict page to match addr
*/
int symbol_dict_find_addr(TSymbolVolume *dict, char *label, int *addr) {
	int count;
	TSymbolDictEntry *entries;

	for(int pageIdx= 0; pageIdx < 256; pageIdx++) {
		count= dict->dict[pageIdx].count;
		if(count == 0)
			continue;

		entries= dict->dict[pageIdx].entries;
		for(int entryIdx= 0; entryIdx < count; entryIdx++) {
			if(!strnicmp(label, entries[entryIdx].label, SYMBOL_LABEL_MAXLEN)) {
				*addr= (pageIdx << 8) | entries[entryIdx].addr8;
				return 1;
			}
		}

	}

	return 0;
}

/*
	lookup in bank dict for label and return addr
*/
int symbol_find_addr(int bank, char *label) {
	TSymbolVolume *volume= symbol_get_bank_volume(bank);
	if(volume == NULL)
		return SYMBOL_NOT_FOUND;

	int addr;
	if(!symbol_dict_find_addr(volume, label, &addr))
		return SYMBOL_NOT_FOUND;

	return addr;
}

/*
	lookup for label and return addr
*/
const char *symbol_lookup(char *label) {
	return symbolDict_get_addresses(label);
}


/*

*/
TSymbolDictEntry *symbol_dict_add_entry(TSymbolVolume *volume, int addr, char *label) {
	int pageIdx= addr >> 8;
	TSymbolDictIndex *dict= &volume->dict[pageIdx];

	if(dict->count == 0) {
		dict->entries= malloc(sizeof(TSymbolDictEntry));
	} else {
		dict->entries= realloc(dict->entries, (dict->count+1) * sizeof(TSymbolDictEntry));
	}

	if(dict->entries == NULL) {
		dict->count= 0;
		return NULL;
	}

	dict->count++;

	TSymbolDictEntry *entry= &dict->entries[dict->count-1];
	memset(entry, 0, sizeof(TSymbolDictEntry));
	entry->addr8= addr & 0xFF;
	strncpy(entry->label, label, 14);

	return entry;
}

/*
	update an existing symbol with <addr, label>
*/
TSymbolDictEntry *symbol_dict_update_entry(TSymbolDictEntry *entry, int addr, char *label) {

	entry->addr8= addr & 0xFF;
	strncpy(entry->label, label, 14);
	entry->label[14]= 0;

	return entry;
}

/*
	add new symbol <addr, label> into dict <bank>
	if bank<0 then it uses the XX0000 of the addr
*/
int symbol_add(int bank, int addr, char *label) {
	int addr24;
	char *addresses;
	char addrStr[7];
	const char *value;

	bank= bank<0 ? addr>>16 : bank;


	addr24= (bank << 16) | addr;
	sprintf(addrStr, "%06X", addr24);
	value= symbolDict_get_addresses(label);
	if(value && !strstr(value, addrStr)) {
		addresses= calloc(1, strlen(value) + 1 + strlen(addrStr) + 1);
		sprintf(addresses, "%s,%s", value, addrStr);
		symbolDict_set_addresses(label, addresses);
		free(addresses);
	}
	else
		symbolDict_set_addresses(label, addrStr);


	addr= addr & 0xFFFF;

	TSymbolVolume *volume= symbol_get_bank_volume(bank);
	if(volume == NULL)
		volume= symbol_add_volume(bank);

	if(volume == NULL)
		return -1;

	TSymbolDictEntry *entry= symbol_dict_find_entry_by_addr(volume, addr);
	if(entry == NULL) {
		symbol_dict_add_entry(volume, addr, label);
		return 1;
	}
	symbol_dict_update_entry(entry, addr, label);

	return 0;
}

/*
	free the bank volume
*/
int symbol_clear_volume(int bank) {
	TSymbolVolume *vol= symbol_get_bank_volume(bank);
	if(!vol)
		return -1;

	int count= 0;
	for(int idx= 0; idx < 256; idx++) {
		TSymbolDictIndex *dict= &vol->dict[idx];
		free(dict->entries);
		count+= dict->count;
		dict->count= 0;
	}
	return count;
}

/*
	load default symbols file
*/
SymLoadError symbol_load_default(FILE *fp, int bank, int *addedCount, int *dupCount, int *lineCount) {
	char lineBuffer[255];
	int addr;
	char label[255];

	fseek(fp, 0, 0);

	while(fgets(lineBuffer, sizeof(lineBuffer), fp)) {

		if(2 != sscanf(lineBuffer, "%*s %x %s", &addr, label))
			return SynErr;

		addr= symbol_add(bank, addr, label+1);

		if(addr == 1)
			(*addedCount)++;
		else
			(*dupCount)++;

		(*lineCount)++;
	}

	return NoErr;
}

/*
	load user symbols file
*/
SymLoadError symbol_load_user(FILE *fp, int bank, int *addedCount, int *dupCount, int *lineCount) {
	char lineBuffer[255];
	char *linePtr;
	int addr;
	char label[255];

	while(fgets(lineBuffer, sizeof(lineBuffer), fp)) {

		(*lineCount)++;

		linePtr= ltrim(lineBuffer);
		if((*linePtr == ';') || (*linePtr == 0x00)) {
			continue;
		}

		if(2 != sscanf(linePtr, "%x %s", &addr, label))
			return SynErr;

		addr= symbol_add(bank, addr, label);

		if(addr == 1)
			(*addedCount)++;
		else
			(*dupCount)++;

	}

	return NoErr;
}

/*
	load symbols file
*/
SymLoadError symbol_load(char *filename, int bank, int *addedCount, int *dupCount, int *lineCount) {
	FILE *fp;
	char lineBuffer[255];
	SymLoadError err;

	fp= fopen(filename, "r");
	if(fp == NULL)
        return CantOpen;

	fgets(lineBuffer, sizeof(lineBuffer), fp);
	SymFileType fileType= !strncmp("X16_SYMBOLS", lineBuffer, 11) ? User : Default;

	switch(fileType) {
		case Default:	err= symbol_load_default(fp, bank, addedCount, dupCount, lineCount); break;
		case User:		err= symbol_load_user(fp, bank, addedCount, dupCount, lineCount); break;
	}

	fclose(fp);

	return err;
}

/*

*/
void symbol_dump(char *filename) {
	FILE *fp= fopen(filename, "w");
	if(fp == NULL)
        return;

	dictionary_dump(symbolsDict, fp);

	fclose(fp);

}

/*

*/
void symbol_init() {
	symbolsDict= dictionary_new(0);
	dictionary_set(symbolsDict, "symbol", NULL);
	dictionary_set(symbolsDict, "var", NULL);
}

/*
	load the symbol table for the specified bank
	symload bank symbolFilename
*/
void symbol_free() {
	TSymbolVolume *vol;

	for(int idx= 0; (vol= symbol_get_volume(idx)); idx++) {
		symbol_clear_volume(vol->bank);
	}

	free(volumes);

	dictionary_del(symbolsDict);
}
