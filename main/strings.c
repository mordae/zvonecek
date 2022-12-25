/*
 * Copyright (C) 2022 Jan Hamal Dvořák <mordae@anilinux.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include "strings.h"


#define NOTE_C  261.6256
#define NOTE_Cs 277.1826
#define NOTE_D  293.6648
#define NOTE_Ds 311.1270
#define NOTE_E  329.6276
#define NOTE_F  349.2282
#define NOTE_Fs 369.9944
#define NOTE_G  391.9954
#define NOTE_Gs 415.3047
#define NOTE_A  440.0000
#define NOTE_As 466.1638
#define NOTE_H  493.8833


struct synth_string strings_piano1[NUM_STRINGS] = {
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_C,
		.feedback = 0.50,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_Cs,
		.feedback = 0.55,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_D,
		.feedback = 0.60,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_Ds,
		.feedback = 0.64,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_E,
		.feedback = 0.68,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_F,
		.feedback = 0.70,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_Fs,
		.feedback = 0.72,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_G,
		.feedback = 0.74,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_Gs,
		.feedback = 0.76,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_A,
		.feedback = 0.78,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_As,
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_H,
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_C * 2),
		.feedback = 0.80,
		.decay = 0.999,
	},
};


struct synth_string strings_piano2[NUM_STRINGS] = {
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_C * 2),
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_Cs * 2),
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_D * 2),
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_Ds * 2),
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_E * 2),
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_F * 2),
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_Fs * 2),
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_G * 2),
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_Gs * 2),
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_A * 2),
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_As * 2),
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_H * 2),
		.feedback = 0.80,
		.decay = 0.999,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_C * 4),
		.feedback = 0.80,
		.decay = 0.999,
	},
};

struct synth_string *strings_current = strings_piano2;
