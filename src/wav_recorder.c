// Commander X16 Emulator
// Copyright (c) 2022 Stephen Horn
// All rights reserved. License: 2-clause BSD

#include "wav_recorder.h"

#include "SDL.h"
#include "audio.h"
#include "glue.h"
#include <string.h>
#include <stdlib.h>

#pragma pack(push, 1)
typedef struct {
	char     chunk_id[4];
	uint32_t size;
	char     wave_id[4];
} riff_chunk;

typedef struct {
	char     chunk_id[4];
	uint32_t size;
	uint16_t format_tag;
	uint16_t channels;
	uint32_t samples_per_sec;
	uint32_t bytes_per_sec;
	uint16_t block_align;
	uint16_t bits_per_sample;
} fmt_chunk;

typedef struct {
	char     chunk_id[4];
	uint32_t size;
} data_chunk;

typedef struct {
	riff_chunk riff;
	fmt_chunk  fmt;
	data_chunk data;
} file_header;
#pragma pack(pop)

static SDL_RWops * wav_file = NULL;
static file_header wav_header;
static uint32_t    wav_samples_written = 0;

static void
wav_init_file_header(file_header *header)
{
	header->riff.chunk_id[0]    = 'R';
	header->riff.chunk_id[1]    = 'I';
	header->riff.chunk_id[2]    = 'F';
	header->riff.chunk_id[3]    = 'F';
	header->riff.size           = 4;
	header->riff.wave_id[0]     = 'W';
	header->riff.wave_id[1]     = 'A';
	header->riff.wave_id[2]     = 'V';
	header->riff.wave_id[3]     = 'E';
	header->fmt.chunk_id[0]     = 'f';
	header->fmt.chunk_id[1]     = 'm';
	header->fmt.chunk_id[2]     = 't';
	header->fmt.chunk_id[3]     = ' ';
	header->fmt.size            = 16;
	header->fmt.format_tag      = 0x0001; // WAVE_FORMAT_PCM
	header->fmt.channels        = 2;
	header->fmt.samples_per_sec = 0;
	header->fmt.bytes_per_sec   = 0;
	header->fmt.block_align     = 0;
	header->fmt.bits_per_sample = 16 * 2;
	header->data.chunk_id[0]    = 'd';
	header->data.chunk_id[1]    = 'a';
	header->data.chunk_id[2]    = 't';
	header->data.chunk_id[3]    = 'a';
	header->data.size           = 0;
}

static void
wav_update_sizes()
{
	wav_header.data.size = sizeof(int16_t) * wav_header.fmt.channels * wav_samples_written;
	wav_header.riff.size = 4 + sizeof(fmt_chunk) + sizeof(data_chunk) + (wav_header.data.size);
}

static void
wav_begin(const char *path, int32_t sample_rate)
{
	wav_file = SDL_RWFromFile(path, "wb");
	if (wav_file) {
		wav_init_file_header(&wav_header);
		wav_header.fmt.samples_per_sec = sample_rate;
		wav_header.fmt.bytes_per_sec   = sample_rate * sizeof(int16_t) * wav_header.fmt.channels;
		wav_header.fmt.block_align     = sizeof(int16_t) * wav_header.fmt.channels;
		wav_header.fmt.bits_per_sample = (sizeof(int16_t)) << 3;

		const size_t written = SDL_RWwrite(wav_file, &wav_header, sizeof(file_header), 1);
		if (written == 0) {
			SDL_RWclose(wav_file);
			wav_file = NULL;
		}
	}
}

static void
wav_end()
{
	if (wav_file != NULL) {
		wav_update_sizes();
		SDL_RWseek(wav_file, 0, RW_SEEK_SET);
		SDL_RWwrite(wav_file, &wav_header, sizeof(file_header), 1);
		SDL_RWclose(wav_file);
		wav_file = NULL;
	}
}

static void
wav_add(const int16_t *samples, const int num_samples)
{
	if (wav_file) {
		const size_t bytes   = sizeof(int16_t) * 2 * num_samples;
		const size_t written = SDL_RWwrite(wav_file, samples, bytes, 1);
		if (written == 0) {
			SDL_RWclose(wav_file);
			wav_file = NULL;
		} else {
			wav_samples_written += num_samples;
		}
	}
}

// WAV recorder states
typedef enum {
	RECORD_WAV_DISABLED = 0,
	RECORD_WAV_PAUSED,
	RECORD_WAV_AUTOSTARTING,
	RECORD_WAV_RECORDING
} wav_recorder_state_t;

static wav_recorder_state_t Wav_record_state = RECORD_WAV_DISABLED;
static char *               Wav_path         = NULL;

void
wav_recorder_shutdown()
{
	if (Wav_record_state == RECORD_WAV_RECORDING) {
		wav_end();
	}
}

void
wav_recorder_process(const int16_t *samples, const int num_samples)
{
	int i;
	if (Wav_record_state == RECORD_WAV_AUTOSTARTING) {
		for (i = 0; i < num_samples; ++i) {
			if (samples[i] != 0) {
				Wav_record_state = RECORD_WAV_RECORDING;
				wav_begin(Wav_path, host_sample_rate);
				break;
			}
		}
	}

	if (Wav_record_state == RECORD_WAV_RECORDING) {
		wav_add(samples, num_samples);
	}
}

void
wav_recorder_set(wav_recorder_command_t command)
{
	if (Wav_record_state != RECORD_WAV_DISABLED) {
		switch (command) {
			case RECORD_WAV_PAUSE:
				Wav_record_state = RECORD_WAV_PAUSED;
				break;
			case RECORD_WAV_RECORD:
				Wav_record_state = RECORD_WAV_RECORDING;
				wav_begin(Wav_path, host_sample_rate);
				break;
			case RECORD_WAV_AUTOSTART:
				Wav_record_state = RECORD_WAV_AUTOSTARTING;
				break;
			default:
				printf("Unknown command %d passed to wav_recorder_set.\n", (int)command);
				break;
		}
	}
}

uint8_t
wav_recorder_get_state()
{
	return (uint8_t)Wav_record_state;
}

void
wav_recorder_set_path(const char *path)
{
	if (Wav_record_state == RECORD_WAV_RECORDING) {
		wav_end();
	}

	if (Wav_path != NULL) {
		free(Wav_path);
		Wav_path = NULL;
	}

	if (path != NULL) {
		Wav_path = malloc(sizeof(char) * (strlen(path) + 1));
		strcpy(Wav_path, path);

		if (!strcmp(Wav_path + strlen(Wav_path) - 5, ",wait")) {
			Wav_path[strlen(Wav_path) - 5] = 0;
			Wav_record_state               = RECORD_WAV_PAUSED;
		} else if (!strcmp(Wav_path + strlen(Wav_path) - 5, ",auto")) {
			Wav_path[strlen(Wav_path) - 5] = 0;
			Wav_record_state               = RECORD_WAV_AUTOSTARTING;
		} else {
			Wav_record_state = RECORD_WAV_RECORDING;
			wav_begin(Wav_path, host_sample_rate);
		}
	} else {
		Wav_record_state = RECORD_WAV_DISABLED;
	}
}
