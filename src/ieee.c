// Commander X16 Emulator
// Copyright (c) 2022 Michael Steil
// All rights reserved. License: 2-clause BSD

// Commodore Bus emulation
// * L2: TALK/LISTEN layer: https://www.pagetable.com/?p=1031
// * L3: Commodore DOS: https://www.pagetable.com/?p=1038
// This is used from
// * serial.c: L1: Serial Bus emulation (low level)
// * main.c: IEEE KERNAL call hooks (high level)

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <SDL.h>
#include <errno.h>
#include <time.h>
#include "memory.h"
#include "ieee.h"
#include "glue.h"
#ifdef __MINGW32__
#include <direct.h>
// realpath doesn't exist on Windows, but _fullpath is similar enough
#define realpath(N,R) _fullpath((R),(N),PATH_MAX)
// Windows just has to be different
#define localtime_r(S,D) !localtime_s(D,S)
#endif

extern SDL_RWops *prg_file;

#define UNIT_NO 8

// Globals

//bool log_ieee = true;
bool log_ieee = false;

char error[80];
int error_len = 0;
int error_pos = 0;
char cmd[80];
int cmdlen = 0;
int namelen = 0;
int channel = 0;
bool listening = false;
bool talking = false;
bool opening = false;
bool overwrite = false;
bool path_exists = false;

char *hostfscwd = NULL;

uint8_t dirlist[1024]; // Plenty large to hold a single entry
int dirlist_len = 0;
int dirlist_pos = 0;
bool dirlist_cwd = false; // whether we're doing a cwd dirlist or a normal one
bool dirlist_eof = true;
bool dirlist_timestmaps = false;
DIR *dirlist_dirp;
char dirlist_wildcard[256];
char dirlist_type_filter;

const char *blocks_free = "BLOCKS FREE.";

typedef struct {
	char name[80];
	bool read;
	bool write;
	SDL_RWops *f;
} channel_t;

channel_t channels[16];

// Prototypes for some of the static functions

static void clear_error();
static void set_error(int e, int t, int s);
static void cchdir(char *dir);
static int cgetcwd(char *buf, size_t len);
static void cseek(int channel, uint32_t pos);
static void cmkdir(char *dir);
static void crmdir(char *dir);
static void cunlink(char *f);
static void crename(char *f);

// Functions

// Puts the emulated cwd in buf, up to the maximum length specified by len
// Turn null termination into a space
// This is for displaying in the directory header
static int
cgetcwd(char *buf, size_t len)
{
	int o = 0;
	char l = fsroot_path[strlen(fsroot_path)-1];
	if (l == '/' || l == '\\')
		o--;
	if (strlen(fsroot_path) == strlen(hostfscwd))
		strncpy(buf, "/", len);
	else 
		strncpy(buf, hostfscwd+strlen(fsroot_path)+o, len);
	// Turn backslashes into slashes
	for (o = 0; o < len; o++) {
		if (buf[o] == 0) buf[o] = ' ';
		if (buf[o] == '\\') buf[o] = '/';
	}

	return 0;
}

static char *
parse_dos_filename(const char *name)
{
	// in case the name starts with something with special meaning,
	// such as @0:
	char *name_ptr;
	char *newname = malloc(strlen(name)+1);
	int i, j;

	newname[strlen(name)+1] = 0;
	
	overwrite = false;

	// [[@][<media 0-9>][</relative_path/> | <//absolute_path/>]:]<file_path>[*]
	// Examples of valid dos filenames
	//   ":FILE.PRG"  (same as "FILE.PRG")
	//   "@:FILE.PRG"  (same as "FILE.PRG" but overwrite okay)
	//   "@0:FILE.PRG"  (same as above)
	//   "//DIR/:FILE.PRG"  (same as "/DIR/FILE.PRG")
	//   "/DIR/:FILE.PRG"  (same as "./DIR/FILE.PRG")
	//   "FILE*" (matches the first file in the directory which starts with "FILE")

	// This routine only parses the bits before the ':'
	// and normalizes directory parts by attaching them to the name part

	// resolve_path() is responsible for resolving absolute and relative
	// paths, and for processing the wildcard option


	if ((name_ptr = strchr(name,':'))) {
		name_ptr++;
		i = 0;
		j = 0;

		// @ is overwrite flag
		if (name[i] == '@') {
			overwrite = true;
			i++;
		}

		// Medium, we don't care what this number is
		while (name[i] >= '0' && name[i] <= '9')
			i++;


		// Directory name
		if (name[i] == '/') {
			i++;
			for (; name+i+1 < name_ptr; i++, j++) {
				newname[j] = name[i];
			}

			// Directory portion must end with /
			if (newname[j-1] != '/') {
				free(newname);
				return NULL;
			}
		}

		strcpy(newname+j,name_ptr);

	} else {
		strcpy(newname,name);
	}

	return newname;
}

// Returns a ptr to malloc()ed space which must be free()d later, or NULL
static char *
resolve_path(const char *name, bool must_exist)
{
	path_exists = false;
	clear_error();
	// Resolve the filename in the context of the emulated cwd
	// allocate plenty of string space
	char *tmp = malloc(strlen(name)+strlen(hostfscwd)+2);
	char *c;
	char *d;
	char *ret;
	DIR *dirp;
	struct dirent *dp;
	int i;

	if (tmp == NULL) {
		set_error(0x70, 0, 0);
		return NULL;
	}

	// If the filename begins with /, simply append it to the fsroot_path,
	// slash(es) and all, otherwise append it to the cwd, but with a /
	// in between
	if (name[0] == '/' || name[0] == '\\') { // absolute
		strcpy(tmp, fsroot_path);
		strcpy(tmp+strlen(fsroot_path), name);
	} else { // relative
		strcpy(tmp, hostfscwd);
		tmp[strlen(hostfscwd)] = '/';
		strcpy(tmp+strlen(hostfscwd)+1, name);
	}

	if (strchr(tmp,'*') || strchr(tmp,'?')) { // oh goodie, a wildcard
		// we have to search the directory for the first occurrence
		// in directory order

		c = strrchr(tmp,'\\');
		d = strrchr(tmp,'/');

		if (c == NULL && d == NULL) { // This should never happen
			free(tmp);
			set_error(0x62, 0, 0);
			return NULL;
		}

		// Chop off ret at the last path separator
		// and set c to point at the element with the wildcard
		// pattern
		if (c > d) {
			*c = 0;
			c++;
		} else {
			*d = 0;
			c = d+1;
		}

		if (!(dirp = opendir(tmp))) { // Directory couldn't be opened
			free(tmp);
			set_error(0x62, 0, 0);
			return NULL;
		}

		ret = NULL;

		bool found = false;

		while ((dp = readdir(dirp))) {
			// in a wildcard match that starts at first position, leading dot filenames are not considered
			if ((*c == '*' || *c == '?') && *(dp->d_name) == '.')
				continue;
			for (i = 0; i < strlen(c) && i < strlen(dp->d_name); i++) {
				if (c[i] == '*') {
					found = true;
					break;
				} else if (c[i] == '?') {
					continue;
				} else if (c[i] != (dp->d_name)[i]) {
					break;
				}
			}

			// If we reach the end of both strings, it's a match
			if (i == strlen(dp->d_name) && i == strlen(c)) 
				found = true;

			if (found) { // simple wildcard match
				ret = malloc(strlen(tmp)+strlen(dp->d_name)+2);
				if (ret == NULL) { // memory allocation error
					free(tmp);
					closedir(dirp);
					set_error(0x70, 0, 0);
					return NULL;
				}
				strcpy(ret, tmp);
				ret[strlen(tmp)] = '/';
				strcpy(ret+strlen(tmp)+1, dp->d_name);
				free(tmp);
				break;
			}
		}

		closedir(dirp);

		if (!ret) { // No wildcard match
			free(tmp);
			set_error(0x62, 0, 0);
			return NULL;
		}

		tmp = ret; // section after this expects the topic to be in tmp

	}


	// now resolve the path using OS routines
	ret = realpath(tmp, NULL);
	free(tmp);

	if (ret == NULL) {
		if (must_exist) {
			// path wasn't found or had another error in construction
			set_error(0x62, 0, 0);
		} else {
			// path does not exist, but as long as everything but the final
			// path element exists, we're still okay.
			tmp = malloc(strlen(name)+1);
			if (tmp == NULL) {
				set_error(0x70, 0, 0);
				return NULL;
			}
			strcpy(tmp, name);
			c = strrchr(tmp, '/');
			if (c == NULL)
				c = strrchr(tmp, '\\');
			if (c != NULL)
				*c = 0; // truncate string here

			// assemble a path with what we have left
			ret = malloc(strlen(tmp)+strlen(hostfscwd)+2);
			if (ret == NULL) {
				free(tmp);
				set_error(0x70, 0, 0);
				return NULL;
			}

			if (name[0] == '/' || name[0] == '\\') { // absolute
				strcpy(ret, fsroot_path);
				strcpy(ret+strlen(fsroot_path), tmp);
			} else { // relative
				strcpy(ret, hostfscwd);
				*(ret+strlen(hostfscwd)) = '/';
				strcpy(ret+strlen(hostfscwd)+1, tmp);
			}

			free(tmp);

			// if we found a path separator in the name string
			// we check everything up to that final separator
			if (c != NULL) {
				tmp = realpath(ret, NULL);
				free(ret);
				if (tmp == NULL) {
					// missing parent path element too
					set_error(0x62, 0, 0);
					ret = NULL;
				} else {
					free(tmp);
					// found everything up to the parent path element
					// restore ret to original case
					ret = malloc(strlen(name)+strlen(hostfscwd)+2);
					if (ret == NULL) {
						set_error(0x70, 0, 0);
						return NULL;
					}
					strcpy(ret, hostfscwd);
					ret[strlen(hostfscwd)] = '/';
					strcpy(ret+strlen(hostfscwd)+1, name);
				}
			}
		}
	} else {
		path_exists = true;
	}

	if (ret == NULL)
		return ret;


	// Prevent resolving outside the fsroot_path
	if (strlen(fsroot_path) > strlen(ret)) {
		free(ret);
		set_error(0x62, 0, 0);
		return NULL;
	} else if (strncmp(fsroot_path, ret, strlen(fsroot_path))) {
		free(ret);
		set_error(0x62, 0, 0);
		return NULL;
	} else if (strlen(fsroot_path) < strlen(ret) &&
	           fsroot_path[strlen(fsroot_path)-1] != '/' &&
			   fsroot_path[strlen(fsroot_path)-1] != '\\' &&
	           ret[strlen(fsroot_path)] != '/' &&
	           ret[strlen(fsroot_path)] != '\\')
	{
		// ret matches beginning of fsroot_path,
		// end of fsroot_path is not a path-separator,
		// and next char in ret is not a path-separator
		// This could happen if
		//   fsroot_path == "/home/user/ba" and
		//   ret == "/home/user/bah".
		// This condition should be considered a jailbreak and we fail out
		free(ret);
		set_error(0x62, 0, 0);
		return NULL;
	}

	return ret;
}



static int
create_directory_listing(uint8_t *data, char *dirstring)
{
	uint8_t *data_start = data;

	dirlist_eof = true;
	dirlist_cwd = false;
	int i = 1;
	int j;

	
	dirlist_timestmaps = false;
	dirlist_type_filter = 0;
	dirlist_wildcard[0] = 0;

	// Here's where we parse out directory listing options
	// Such as "$=T:MATCH*=P"

	// position 0 is assumed to be $
	// so i starts at 1
	while (i < strlen(dirstring)) {
		if (dirstring[i] == '=') {
			i++;
			if (dirstring[i] == 'T')
				dirlist_timestmaps = true;
		} else if (dirstring[i] == ':') {
			i++;
			j = 0;
			while (i < strlen(dirstring)) {
				if (dirstring[i] == '=' || dirstring[i] == 0) {
					dirlist_wildcard[j] = 0;

					if (dirstring[++i] == 'D')
						dirlist_type_filter = 'D';
					else if (dirstring[i] == 'P')
						dirlist_type_filter = 'P';

					break;
				} else {
					dirlist_wildcard[j++] = dirstring[i++];
				}
			}
		}
		i++;
	}

	// load address
	*data++ = 1;
	*data++ = 8;
	// link
	*data++ = 1;
	*data++ = 1;
	// line number
	*data++ = 0;
	*data++ = 0;
	*data++ = 0x12; // REVERSE ON
	*data++ = '"';
	for (int i = 0; i < 16; i++) {
		*data++ = ' ';
	}
	if (cgetcwd((char *)data - 16, 16)) {
		return false;
	}
	*data++ = '"';
	*data++ = ' ';
	*data++ = 'H';
	*data++ = 'O';
	*data++ = 'S';
	*data++ = 'T';
	*data++ = ' ';
	*data++ = 0;

	if (!(dirlist_dirp = opendir(hostfscwd))) {
		return 0;
	}
	dirlist_eof = false;
	return data - data_start;
}

static int
continue_directory_listing(uint8_t *data)
{
	uint8_t *data_start = data;
	struct stat st;
	struct dirent *dp;
	int file_size;
	char *tmpnam;
	bool found;
	int i;

	while ((dp = readdir(dirlist_dirp))) {
		size_t namlen = strlen(dp->d_name);
		tmpnam = resolve_path(dp->d_name, true);
		if (tmpnam == NULL) continue;
		stat(tmpnam, &st);
		free(tmpnam);

		// Type match
		if (dirlist_type_filter) {
			switch (dirlist_type_filter) {
				case 'D':
					if (!S_ISDIR(st.st_mode))
						continue;
					break;
				case 'P':
					if (!S_ISREG(st.st_mode))
						continue;
					break;
			}
		}

		if (dirlist_wildcard[0]) { // wildcard match selected
			// in a wildcard match that starts at first position, leading dot filenames are not considered
			if ((dirlist_wildcard[0] == '*' || dirlist_wildcard[0] == '?') && *(dp->d_name) == '.')
				continue;

			found = false;
			for (i = 0; i < strlen(dirlist_wildcard) && i < strlen(dp->d_name); i++) {
				if (dirlist_wildcard[i] == '*') {
					found = true;
					break;
				} else if (dirlist_wildcard[i] == '?') {
					continue;
				} else if (dirlist_wildcard[i] != (dp->d_name)[i]) {
					break;
				}
			}

			// If we reach the end of both strings, it's a match
			if (i == strlen(dp->d_name) && i == strlen(dirlist_wildcard)) 
				found = true;

			if (!found) continue;
		}


		file_size = (st.st_size + 255)/256;
		if (file_size > 0xFFFF) {
			file_size = 0xFFFF;
		}

		// link
		*data++ = 1;
		*data++ = 1;

		*data++ = file_size & 0xFF;
		*data++ = file_size >> 8;
		if (file_size < 1000) {
			*data++ = ' ';
			if (file_size < 100) {
				*data++ = ' ';
				if (file_size < 10) {
					*data++ = ' ';
				}
			}
		}
		*data++ = '"';
		//if (namlen > 16) {
		//	namlen = 16; // TODO hack
		//}
		memcpy(data, dp->d_name, namlen);
		data += namlen;
		*data++ = '"';
		for (int i = namlen; i < 16; i++) {
			*data++ = ' ';
		}
		*data++ = ' ';
		if (S_ISDIR(st.st_mode)) {
			*data++ = 'D';
			*data++ = 'I';
			*data++ = 'R';
		} else {
			*data++ = 'P';
			*data++ = 'R';
			*data++ = 'G';
		}
		// This would be a '<' if file were protected, but it's a space instead
		*data++ = ' ';

		if (dirlist_timestmaps) {
			// ISO-8601 date+time
			struct tm mtime;
			if (localtime_r(&st.st_mtime, &mtime)) {
				*data++ = ' '; // space before the date
				data += strftime((char *)data, 20, "%Y-%m-%d %H:%M:%S", &mtime);
			}
		}
		
		*data++ = 0;
		return data - data_start;
	}

	// link
	*data++ = 1;
	*data++ = 1;

	*data++ = 255; // "65535"
	*data++ = 255;

	memcpy(data, blocks_free, strlen(blocks_free));
	data += strlen(blocks_free);
	*data++ = 0;

	// link
	*data++ = 0;
	*data++ = 0;
	(void)closedir(dirlist_dirp);
	dirlist_eof = true;
	return data - data_start;
}


static int
create_cwd_listing(uint8_t *data)
{
	uint8_t *data_start = data;
	int file_size;

	// load address
	*data++ = 1;
	*data++ = 8;
	// link
	*data++ = 1;
	*data++ = 1;
	// line number
	*data++ = 0;
	*data++ = 0;
	*data++ = 0x12; // REVERSE ON
	*data++ = '"';
	for (int i = 0; i < 16; i++) {
		*data++ = ' ';
	}
	if (cgetcwd((char *)data - 16, 16)) {
		dirlist_eof = true;
		return 0;
	}
	*data++ = '"';
	*data++ = ' ';
	*data++ = 'H';
	*data++ = 'O';
	*data++ = 'S';
	*data++ = 'T';
	*data++ = ' ';
	*data++ = 0;

	char *tmp = malloc(strlen(hostfscwd)+1);
	if (tmp == NULL) {
		set_error(0x70, 0, 0);
		return 0;
	}
	int i = strlen(hostfscwd);
	int j = strlen(fsroot_path);
	strcpy(tmp,hostfscwd);

	for(;;) {
		if (i >= j && tmp[i-1] != '/' && tmp[i-1] != '\\') {
			i--;
			continue;
		}
		
		tmp[i-1]=0;

		if (i < j) {
			strcpy(tmp+i,"/");
		}

		file_size = 0;
		size_t namlen = strlen(tmp+i);

		if (!namlen) continue; // there was a doubled path separator

		// link
		*data++ = 1;
		*data++ = 1;

		*data++ = file_size & 0xFF;
		*data++ = file_size >> 8;
		if (file_size < 1000) {
			*data++ = ' ';
			if (file_size < 100) {
				*data++ = ' ';
				if (file_size < 10) {
					*data++ = ' ';
				}
			}
		}
		*data++ = '"';
//		if (namlen > 16) {
//			namlen = 16; // TODO hack
//		}
		memcpy(data, tmp+i, namlen);
		data += namlen;
		*data++ = '"';
		for (int i = namlen; i < 16; i++) {
			*data++ = ' ';
		}
		*data++ = ' ';
		*data++ = 'D';
		*data++ = 'I';
		*data++ = 'R';
		*data++ = 0;

		if (i < j) break;
		i--;
	}

	free(tmp);

	// link
	*data++ = 1;
	*data++ = 1;

	*data++ = 255; // "65535"
	*data++ = 255;

	memcpy(data, blocks_free, strlen(blocks_free));
	data += strlen(blocks_free);
	*data++ = 0;

	// link
	*data++ = 0;
	*data++ = 0;

	dirlist_eof = true;
	dirlist_cwd = true;

	return data - data_start;
}



static char*
error_string(int e)
{
	switch(e) {
		case 0x00:
			return " OK";
		case 0x01:
			return " FILES SCRATCHED";
		case 0x02:
			return "PARTITION SELECTED";
		// 0x2x: Physical disk error
		case 0x20:
			return "READ ERROR"; // generic read error
		case 0x25:
			return "WRITE ERROR"; // generic write error
		case 0x26:
			return "WRITE PROTECT ON";
		// 0x3x: Error parsing the command
		case 0x30: // generic
		case 0x31: // invalid command
		case 0x32: // command buffer overflow
			return "SYNTAX ERROR";
		case 0x33: // illegal filename
			return "ILLEGAL FILENAME";
		case 0x34: // empty file name
			return "EMPTY FILENAME";
		case 0x39: // subdirectory not found
			return "SUBDIRECTORY NOT FOUND";
		// 0x4x: Controller error (CMD addition)
		case 0x49:
			return "INVALID FORMAT"; // partition present, but not FAT32
		// 0x5x: Relative file related error
		// unsupported
		// 0x6x: File error
		case 0x62:
			return " FILE NOT FOUND";
		case 0x63:
			return "FILE EXISTS";
		// 0x7x: Generic disk or device error
		case 0x70:
			return "NO CHANNEL"; // error allocating context
		case 0x71:
			return "DIRECTORY ERROR"; // FAT error
		case 0x72:
			return "PARTITION FULL"; // filesystem full
		case 0x73:
			return "HOST FS V1.0 X16";
		case 0x74:
			return "DRIVE NOT READY"; // illegal partition for any command but "CP"
		case 0x75:
			return "FORMAT ERROR";
		case 0x77:
			return "SELECTED PARTITION ILLEGAL";
		default:
			return "";
	}
}

static void
set_error(int e, int t, int s)
{
	snprintf(error, sizeof(error), "%02x,%s,%02d,%02d\r", e, error_string(e), t, s);
	error_len = strlen(error);
	error_pos = 0;
}

static void
clear_error()
{
	set_error(0, 0, 0);
}


static void
command(char *cmd)
{
	if (!cmd[0]) {
		return;
	}
	if (log_ieee) {
		if (cmd[0] == 'P') {
			printf("  COMMAND \"%c\" [binary parameters suppressed]\n", cmd[0]);
		} else {
			printf("  COMMAND \"%s\"\n", cmd);
		}
	}
	switch(cmd[0]) {
		case 'C': // C (copy), CD (change directory), CP (change partition)
			switch(cmd[1]) {
				case 'D': // Change directory
					if (cmd[2] == ':') {
						cchdir(cmd+3);
						return;
					}
				case 'P': // Change partition
					set_error(0x02, 0, 0);
					return;
				default: // Copy
					// NYI
					set_error(0x30, 0, 0);
					return;
			}
		case 'I': // Initialize
			clear_error();
			return;
		case 'M': // MD
			switch(cmd[1]) {
				case 'D': // Make directory
					if (cmd[2] == ':') {
						cmkdir(cmd+3);
						return;
					}
				default: // Memory (not implemented)
					set_error(0x31, 0, 0);
					return;
			}
		case 'P': // Seek
			cseek(cmd[1],
				((uint8_t)cmd[2])
				| ((uint8_t)cmd[3] << 8)
				| ((uint8_t)cmd[4] << 16)
				| ((uint8_t)cmd[5] << 24));
			return;
		case 'R': // RD
			switch(cmd[1]) {
				case 'D': // Remove directory
					if (cmd[2] == ':') {
						crmdir(cmd+3);
						return;
					}
				default: // Rename 
					crename(cmd); // Need to parse out the arg in this function
					return;
			}
		case 'S':
			switch(cmd[1]) {
				case '-': // Swap
					set_error(0x31, 0, 0);
					return;
				default: // Scratch
					cunlink(cmd); // Need to parse out the arg in this function
					return;
			}	
		case 'U':
			switch(cmd[1]) {
				case 'I': // UI: Reset
					set_error(0x73, 0, 0);
					return;
			}
		default:
			if (log_ieee) {
				printf("    (unsupported command ignored)\n");
			}
	}
	set_error(0x30, 0, 0);
}

static void
cchdir(char *dir)
{
	// The directory name is in dir, coming from the command channel
	// with the CD: portion stripped off
	char *resolved;
	struct stat st;

	if ((resolved = resolve_path(dir, true)) == NULL) {
		// error already set
		return;
	}

	// Is it a directory?
	if (stat(resolved, &st)) {
		// FNF
		free(resolved);
		set_error(0x62, 0, 0);
	} else if (!S_ISDIR(st.st_mode)) {
		// Not a directory
		free(resolved);
		set_error(0x39, 0, 0);
	} else {
		// cwd has been changed
		free(hostfscwd);
		hostfscwd = resolved;
	}

	return;
}

static void
cmkdir(char *dir)
{
	// The directory name is in dir, coming from the command channel
	// with the MD: portion stripped off
	char *resolved;

	clear_error();
	if ((resolved = resolve_path(dir, false)) == NULL) {
		// error already set
		return;
	}
#ifdef __MINGW32__
	if (_mkdir(resolved))
#else
	if (mkdir(resolved,0777))
#endif
	{
		if (errno == EEXIST) {
			set_error(0x63, 0, 0);
		} else {
			set_error(0x62, 0, 0);
		}
	}

	free(resolved);

	return;
}

static void
crename(char *f)
{
	// This function receives the whole R command, which could be
	// "R:NEW=OLD" or "RENAME:NEW=OLD" or anything in between
	// let's simply find the first colon and chop it there
	char *tmp = malloc(strlen(f)+1);
	if (tmp == NULL) {
		set_error(0x70, 0, 0);
		return;
	}
	strcpy(tmp,f);
	char *d = strchr(tmp,':');

	if (d == NULL) {
		// No colon, not a valid rename command
		free(tmp);
		set_error(0x34, 0, 0);
		return;
	}

	d++; // one character after the colon

	// Now split on the = sign to find
	char *s = strchr(d,'=');

	if (s == NULL) {
		// No equals sign, not a valid rename command
		free(tmp);
		set_error(0x34, 0, 0);
		return;
	}

	*(s++) = 0; // null-terminate d and advance s
	
	char *src;
	char *dst;

	clear_error();
	if ((src = resolve_path(s, true)) == NULL) {
		// source not found
		free(tmp);
		set_error(0x62, 0, 0);
		return;
	}

	if ((dst = resolve_path(d, false)) == NULL) {
		// dest not found
		free(tmp);
		free(src);
		set_error(0x39, 0, 0);
		return;
	}

	free(tmp); // we're now done with d and s (part of tmp)

	if (rename(src, dst)) {
		if (errno == EACCES) {
			set_error(0x63, 0, 0);
		} else if (errno == EINVAL) {
			set_error(0x33, 0, 0);
		} else {
			set_error(0x62, 0, 0);
		}
	}

	free(src);
	free(dst);

	return;
}


static void
crmdir(char *dir)
{
	// The directory name is in dir, coming from the command channel
	// with the RD: portion stripped off
	char *resolved;

	clear_error();
	if ((resolved = resolve_path(dir, true)) == NULL) {
		set_error(0x39, 0, 0);
		return;
	}

	if (rmdir(resolved)) {
		if (errno == ENOTEMPTY || errno == EACCES) {
			set_error(0x63, 0, 0);
		} else {
			set_error(0x62, 0, 0);
		}
	}

	free(resolved);

	return;
}


static void
cunlink(char *f)
{
	// This function receives the whole S command, which could be
	// "S:FILENAME" or "SCRATCH:FILENAME" or anything in between
	// let's simply find the first colon and chop it there
	// TODO path syntax and multiple files
	char *tmp = malloc(strlen(f)+1);
	if (tmp == NULL) {
		set_error(0x70, 0, 0);
		return;
	}
	strcpy(tmp,f);
	char *fn = strchr(tmp,':');

	if (fn == NULL) {
		// No colon, not a valid scratch command
		free(tmp);
		set_error(0x34, 0, 0);
		return;
	}

	fn++; // one character after the colon
	char *resolved;

	clear_error();
	if ((resolved = resolve_path(fn, true)) == NULL) {
		free(tmp);
		set_error(0x62, 0, 0);
		return;
	}

	free(tmp); // we're now done with fn (part of tmp)

	if (unlink(resolved)) {
		if (errno == EACCES) {
			set_error(0x63, 0, 0);
		} else {
			set_error(0x62, 0, 0);
		}
	} else {
		set_error(0x01, 0, 0); // 1 file scratched
	}

	free(resolved);

	return;
}


static int
copen(int channel)
{
	if (channel == 15) {
		command(channels[channel].name);
		return -1;
	}

	char *resolved_filename = NULL;
	char *parsed_filename = NULL;
	int ret = -1;

	// decode ",P,W"-like suffix to know whether we're writing
	bool append = false;
	channels[channel].read = true;
	channels[channel].write = false;
	char *first = strchr(channels[channel].name, ',');
	if (first) {
		*first = 0; // truncate name here
		char *second = strchr(first+1, ',');
		if (second) {
			switch(second[1]) {
				case 'A':
					append = true;
					// fallthrough
				case 'W':
					channels[channel].read = false;
					channels[channel].write = true;
					break;
				case 'M':
					channels[channel].read = true;
					channels[channel].write = true;
					break;
			}
		}
	}
	if (channel <= 1) {
		// channels 0 and 1 are magic
		channels[channel].write = channel;
		channels[channel].read = !channel;
	}
	if (log_ieee) {
		printf("  OPEN \"%s\",%d (%s%s)\n", channels[channel].name, channel,
			channels[channel].read ? "R" : "",
			channels[channel].write ? "W" : "");
	}

	if (!channels[channel].write && channels[channel].name[0] == '$') {	
		dirlist_pos = 0;
		if (!strncmp(channels[channel].name,"$=C",3)) {
			// This emulates the behavior in the ROM code in
			// https://github.com/commanderx16/x16-rom/pull/373
			dirlist_len = create_cwd_listing(dirlist);
		} else {
			dirlist_len = create_directory_listing(dirlist, channels[channel].name);
		}
	} else {
		if (!strcmp(channels[channel].name, ":*") && prg_file) {
			channels[channel].f = prg_file; // special case
		} else {
			if ((parsed_filename = parse_dos_filename(channels[channel].name)) == NULL) {
				set_error(0x32, 0, 0); // Didn't parse out properly
				return 2; 
			}

			resolved_filename = resolve_path(parsed_filename, false);
			free(parsed_filename);

			if (resolved_filename == NULL) {
				// Resolve the path, if we get a null ptr back, error is already set.
				return 2;
			}

			if (path_exists && !overwrite && !append && !channels[channel].read) {
				free(resolved_filename);
				set_error(0x63, 0, 0); // forbid overwrite unless requested
				return 2;
			}
		
			if (channels[channel].read && channels[channel].write) {
				channels[channel].f = SDL_RWFromFile(resolved_filename, "rb+");
			} else {
				channels[channel].f = SDL_RWFromFile(resolved_filename, channels[channel].write ? "wb" : "rb");
			}

			free(resolved_filename);
		}

		if (!channels[channel].f) {
			if (log_ieee) {
				printf("  FILE NOT FOUND\n");
			}
			set_error(0x62, 0, 0);
			ret = 2; // FNF
		} else {
			if (append) {
				SDL_RWseek(channels[channel].f, 0, RW_SEEK_END);
			}
			clear_error();
		}
	}
	return ret;
}

static void
cclose(int channel)
{
	if (log_ieee) {
		printf("  CLOSE %d\n", channel);
	}
	channels[channel].name[0] = 0;
	if (channels[channel].f) {
		SDL_RWclose(channels[channel].f);
		channels[channel].f = NULL;
	}
}

static void
cseek(int channel, uint32_t pos)
{
	if (channel == 15) {
		set_error(0x30, 0, 0);
		return;
	}

	if (channels[channel].f) {
		SDL_RWseek(channels[channel].f, pos, RW_SEEK_SET);
	}
}

void
ieee_init()
{
	// Init the hostfs "jail" and cwd
	if (fsroot_path == NULL) { // if null, default to cwd
		// We hold this for the lifetime of the program, and we don't have
		// any sort of destructor, so we rely on the OS teardown to free() it.
		fsroot_path = getcwd(NULL, 0); 
	} else {
		// Normalize it
		fsroot_path = realpath(fsroot_path, NULL);
	}

	if (startin_path == NULL) {
		// same as above
		startin_path = getcwd(NULL, 0);
	} else {
		// Normalize it
		startin_path = realpath(startin_path, NULL);
	}
	// Quick error checks
	if (fsroot_path == NULL) {
		fprintf(stderr, "Failed to resolve argument to -fsroot\n");
		exit(1);
	}

	if (startin_path == NULL) {
		fprintf(stderr, "Failed to resolve argument to -startin\n");
		exit(1);
	}

	// Now we verify that startin_path is within fsroot_path
	// In other words, if fsroot_path is a left-justified substring of startin_path

	// If startin_path is not reachable, we instead default to setting it
	// back to fsroot_path
	if (strncmp(fsroot_path, startin_path, strlen(fsroot_path))) { // not equal
		free(startin_path);
		startin_path = fsroot_path;
	}

	// Now initialize our emulated cwd.
	hostfscwd = malloc(strlen(startin_path)+1);
	if (hostfscwd == NULL) {
		fprintf(stderr, "Failed to allocate memory for hostfscwd\n");
		exit(1);
	}
	strcpy(hostfscwd, startin_path);

	set_error(0x73, 0, 0);
}

void
SECOND(uint8_t a)
{
	if (log_ieee) {
		printf("%s $%02x\n", __func__, a);
	}
	if (listening) {
		channel = a & 0xf;
		opening = false;
		switch(a & 0xf0) {
			case 0x60:
				if (log_ieee) {
					printf("  WRITE %d...\n", channel);
				}
				break;
			case 0xe0:
				cclose(channel);
				break;
			case 0xf0:
				if (log_ieee) {
					printf("  OPEN %d...\n", channel);
				}
				opening = true;
				namelen = 0;
				break;
		}
	}
}

void
TKSA(uint8_t a)
{
	if (log_ieee) {
		printf("%s $%02x\n", __func__, a);
	}
	if (talking) {
		channel = a & 0xf;
	}
}


int
ACPTR(uint8_t *a)
{
	int ret = -1;
	if (channel == 15) {
		if (error_pos >= error_len) {
			clear_error();
		}
		*a = error[error_pos++];
	} else if (channels[channel].read) {
		if (channels[channel].name[0] == '$') {
			if (dirlist_pos < dirlist_len) {
				*a = dirlist[dirlist_pos++];
			} else {
				*a = 0;
			}
			if (dirlist_pos == dirlist_len) {
				if (dirlist_eof) {
					ret = 0x40;
				} else {
					dirlist_pos = 0;
					dirlist_len = continue_directory_listing(dirlist);
				}
			}
		} else if (channels[channel].f) {
			if (SDL_RWread(channels[channel].f, a, 1, 1) != 1) {
				ret = 0x40;
			}
		}
	} else {
		ret = 2; // FNF
	}
	if (log_ieee) {
		printf("%s-> $%02x\n", __func__, *a);
	}
	return ret;
}

int
CIOUT(uint8_t a)
{
	int ret = -1;
	if (log_ieee) {
		printf("%s $%02x\n", __func__, a);
	}
	if (listening) {
		if (opening) {
			if (namelen < sizeof(channels[channel].name) - 1) {
				channels[channel].name[namelen++] = a;
			}
		} else {
			if (channel == 15) {
				// P command takes binary parameters, so we can't terminate
				// the command on CR.
				if ((a == 13) && (cmd[0] != 'P')) {
					cmd[cmdlen] = 0;
					command(cmd);
					cmdlen = 0;
				} else {
					if (cmdlen < sizeof(cmd) - 1) {
						cmd[cmdlen++] = a;
					}
				}
			} else if (channels[channel].write && channels[channel].f) {
				if (!SDL_WriteU8(channels[channel].f, a))
					ret = 0x40;
			} else {
				ret = 2; // FNF
			}
		}
	}
	return ret;
}

void
UNTLK() {
	if (log_ieee) {
		printf("%s\n", __func__);
	}
	talking = false;
}

int
UNLSN() {
	int ret = -1;
	if (log_ieee) {
		printf("%s\n", __func__);
	}
	listening = false;
	if (opening) {
		channels[channel].name[namelen] = 0; // term
		opening = false;
		ret = copen(channel);
	} else if (channel == 15) {
		cmd[cmdlen] = 0;
		command(cmd);
		cmdlen = 0;
	}
	return ret;
}

void
LISTEN(uint8_t a)
{
	if (log_ieee) {
		printf("%s $%02x\n", __func__, a);
	}
	if ((a & 0x1f) == UNIT_NO) {
		listening = true;
	}
}

void
TALK(uint8_t a)
{
	if (log_ieee) {
		printf("%s $%02x\n", __func__, a);
	}
	if ((a & 0x1f) == UNIT_NO) {
		talking = true;
	}
}

int
MACPTR(uint16_t addr, uint16_t *c, uint8_t stream_mode)
{
	int ret = -1;
	int count = *c ?: 256;
	uint8_t ram_bank = read6502(0);
	int i = 0;
	do {
		uint8_t byte = 0;
		ret = ACPTR(&byte);
		if (ret >= 0) {
			break;
		}
		write6502(addr, byte);
		i++;
		if (!stream_mode) {
			addr++;
			if (addr == 0xc000) {
				addr = 0xa000;
				ram_bank++;
				write6502(0, ram_bank);
			}
		}
	} while(i < count);
	*c = i;
	return ret;
}
