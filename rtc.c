// Commander X16 Emulator
// Copyright (c) 2019 Michael Steil
// All rights reserved. License: 2-clause BSD

// This emulates
// * setting and getting the date and time
// * binary/BCD and 12h/24h modes
// * RAM
// It does not emulate the timer, IRQ,
// and various other settings yet.

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include "glue.h"
#include "rtc.h"

unsigned int clocks;

static uint8_t seconds;
static uint8_t seconds_alarm;
static uint8_t minutes;
static uint8_t minutes_alarm;
static uint8_t hours;
static uint8_t hours_alarm;
static uint8_t day_of_week;
static uint8_t day;
static uint8_t month;
static uint8_t year;

static bool h24; // 12/24h: 0 = AM/PM, 1 = 24h
static bool dm;  // data mode: 0 = BCD, 1 = binary
static uint8_t rtc_ram[0x80];

static uint8_t
encode_byte(uint8_t in)
{
	if (!dm) {
		in = ((in / 10) << 4) | (in % 10);
	}
	return in;
}

static uint8_t
encode_byte_hour(uint8_t h)
{
	bool pm = false;

	if (!h24) {
		// AM/PM
		if (h >= 12) {
			pm = true;
			h -= 12;
		}
		if (h == 0) {
			h = 12;
		}
	}

	return encode_byte(h) | (pm << 7);
}

static uint8_t
decode_byte(uint8_t in)
{
	if (!dm) {
		in  = (in >> 4) * 10 + (in & 0xf);
	}
	return in;
}

static uint8_t
decode_byte_hour(uint8_t h)
{
	bool pm = false;

	if (!h24) {
		pm = (h >> 7);
		h &= 0x7f;
	}

	h = decode_byte(h);
	if (!h24 && h == 12) {
		h = 0;
	}
	if (pm) {
		h += 12;
	}
	return h;
}

void
rtc_init()
{
	dm = true;
	h24 = true;

	seconds = 0;
	seconds_alarm = 0;
	minutes = 0;
	minutes_alarm = 0;
	hours = 0;
	hours_alarm = 0;
	day_of_week = 1;
	day = 1;
	month = 1;
	year = 0;

	clocks = 0;
}

static uint8_t days_per_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

void
rtc_step(int c)
{
	clocks += c;
	if (clocks < (MHZ * 1000000)) {
		return;
	}

	clocks -= (MHZ * 1000000);
	seconds++;
	if (seconds < 60) {
		return;
	}

	seconds = 0;
	minutes++;
	if (minutes < 60) {
		return;
	}

	minutes = 0;
	hours++;
	if (hours < 24) {
		return;
	}

	hours = 0;
	day_of_week++;
	if (day_of_week > 7) {
		day_of_week = 1;
	}
	day++;
	uint8_t dpm = days_per_month[month - 1];
	if (month == 2 && !(year & 3)) {
		// the clock does 2000-2099, where the "% 4 == 0" rule applies
		dpm++;
	}
	if (day <= dpm) {
		return;
	}

	day = 1;
	month++;
	if (month <= 12) {
		return;
	}

	month = 1;
	year++;
	if (year == 100) {
		year = 0; // Y2.1K problem! ;-)
	}
}

uint8_t
rtc_read(uint8_t reg)
{
	reg = reg & 0x7f;

	switch (reg) {
		case 0:
			return encode_byte(seconds);
		case 1:
			return encode_byte(seconds_alarm);
		case 2:
			return encode_byte(minutes);
		case 3:
			return encode_byte(minutes_alarm);
		case 4:
			return encode_byte_hour(hours);
		case 5:
			return encode_byte_hour(hours_alarm);
		case 6:
			return encode_byte(day_of_week);
		case 7:
			return encode_byte(day);
		case 8:
			return encode_byte(month);
		case 9:
			return encode_byte(year);
		case 0xb: // control B
			return dm << 2 | h24 << 1;
		default:
			return rtc_ram[reg];
	}
}

void
rtc_write(uint8_t reg, uint8_t value)
{
	reg = reg & 0x7f;

	switch (reg) {
		case 0:
			seconds = decode_byte(value);
			break;
		case 1:
			seconds_alarm = decode_byte(value);
			break;
		case 2:
			minutes = decode_byte(value);
			break;
		case 3:
			minutes_alarm = decode_byte(value);
			break;
		case 4:
			hours = decode_byte_hour(value);
			break;
		case 5:
			hours_alarm = decode_byte_hour(value);
			break;
		case 6:
			day_of_week = decode_byte(value);
			break;
		case 7:
			day = decode_byte(value);
			break;
		case 8:
			month = decode_byte(value);
			break;
		case 9:
			year = decode_byte(value);
			break;
		case 0xb: // control B
			dm = !!(value & 4);
			h24 = !!(value & 2);
			break;
		default:
			rtc_ram[reg] = value;
			break;
	}
}
