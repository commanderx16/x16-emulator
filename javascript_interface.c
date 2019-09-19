// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

#include "glue.h"

void 
j2c_reset()
{
    machine_reset();
}

void 
j2c_paste(char * buffer)
{
    machine_paste(buffer);
}
