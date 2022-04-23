// Commander X16 Emulator
// Copyright (c) 2022 Stephen Horn
// All rights reserved. License: 2-clause BSD

#ifndef WAV_RECORDER_H
#define WAV_RECORDER_H

#include <stdint.h>

typedef enum {
	RECORD_WAV_PAUSE = 0,
	RECORD_WAV_RECORD,
	RECORD_WAV_AUTOSTART,
} wav_recorder_command_t;

void wav_recorder_shutdown();
void wav_recorder_process(const int16_t *samples, const int num_samples);

void    wav_recorder_set(wav_recorder_command_t command);
uint8_t wav_recorder_get_state();

void wav_recorder_set_path(const char *path);

#endif
