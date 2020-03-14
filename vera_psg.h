// Commander X16 Emulator
// Copyright (c) 2020 Frank van den Hoef
// All rights reserved. License: 2-clause BSD

#pragma once

#include <stdint.h>

void psg_reset(void);
void psg_writereg(uint8_t reg, uint8_t val);
void psg_render(int16_t *buf, unsigned num_samples);
