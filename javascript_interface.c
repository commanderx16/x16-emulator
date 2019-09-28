// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glue.h"

char javascript_text_data[65536];

void
j2c_reset()
{
	machine_reset();
}

void
j2c_paste(char * buffer)
{
	memset(javascript_text_data, 0, 65536);
	strcpy(javascript_text_data, buffer);
	machine_paste(javascript_text_data);
}

void
j2c_start_audio()
{

#ifdef WITH_YM2151
	init_audio();
#endif

}
