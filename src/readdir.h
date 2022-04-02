// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#ifndef READDIR_H_INCLUDED
#define READDIR_H_INCLUDED

#include <sys/types.h>

#ifdef _WIN32

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

struct dirent
{
	char d_name[MAX_PATH];
};

typedef struct dir_private DIR;

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dir);
int closedir(DIR *dir);

#else
#include <dirent.h>
#endif

#endif /* READDIR_H_INCLUDED */
