/*
 * Copyright (C)  Jan Hamal Dvořák <mordae@anilinux.org>
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

#include "instrument.h"
#include "registry.h"
#include "strings.h"

#include "esp_log.h"

#include <stdio.h>


static const char *tag = "instrument";


static void piano1_enable(void)
{
}


static void piano1_key_press(int key)
{
	synth_string_pluck(&strings_piano1[key]);
}


static void piano1_key_release(int key)
{
	synth_string_dampen(&strings_piano1[key]);
}


static void piano1_read(float *out, size_t len)
{
	for (int i = 0; i < NUM_NOTES; i++) {
		synth_string_read(&strings_piano1[i], out, len);
		synth_string_read(&strings_piano2[i], out, len);
	}
}


struct instrument Piano1 = {
	.enable = piano1_enable,
	.key_press = piano1_key_press,
	.key_release = piano1_key_release,
	.read = piano1_read,
};


static void piano2_enable(void)
{
}


static void piano2_key_press(int key)
{
	synth_string_pluck(&strings_piano2[key]);
}


static void piano2_key_release(int key)
{
	synth_string_dampen(&strings_piano2[key]);
}


static void piano2_read(float *out, size_t len)
{
	for (int i = 0; i < NUM_NOTES; i++) {
		synth_string_read(&strings_piano1[i], out, len);
		synth_string_read(&strings_piano2[i], out, len);
	}
}


struct instrument Piano2 = {
	.enable = piano2_enable,
	.key_press = piano2_key_press,
	.key_release = piano2_key_release,
	.read = piano2_read,
};


static FILE *extras_fp[NUM_NOTES] = {NULL};

static const char *samples[NUM_NOTES] = {
	"/data/toilet.wav",
	"/data/bark.wav",
	"/data/knock.wav",
	"/data/meow.wav",
	"/data/cat.wav",
	"/data/fart.wav",
	"/data/frog.wav",
	"/data/chainsaw.wav",
	"/data/rooster.wav",
	"/data/crying.wav",
	"/data/chicken.wav",
	"/data/glass.wav",
	"/data/plate.wav",
};


static void extras_enable(void)
{
}


static void extras_key_press(int key)
{
	ESP_LOGI(tag, "Play sample %s", samples[key]);

	if (NULL == extras_fp[key]) {
		extras_fp[key] = fopen(samples[key], "rb");
		assert (NULL != extras_fp[key]);
	}

	fseek(extras_fp[key], 44, SEEK_SET);
}


static void extras_key_release(int key)
{
}


static void extras_read(float *out, size_t len)
{
	for (int key = 0; key < NUM_NOTES; key++) {
		if (NULL == extras_fp[key])
			continue;

		int16_t buf[len];
		size_t rd = fread(buf, 2, len, extras_fp[key]);

		for (int i = 0; i < rd; i++)
			out[i] += buf[i];

		if (rd < len) {
			fclose(extras_fp[key]);
			extras_fp[key] = NULL;
		}
	}
}


struct instrument Extras = {
	.enable = extras_enable,
	.key_press = extras_key_press,
	.key_release = extras_key_release,
	.read = extras_read,
};


struct instrument *instrument = &Piano2;


void instrument_select(struct instrument *inst)
{
	instrument = inst;
	instrument->enable();
}


void instrument_next(void)
{
	if (instrument == &Piano1)
		goto select_piano2;
	else if (instrument == &Piano2)
		goto select_extras;
	else if (instrument == &Extras)
		goto select_piano1;

select_piano1:
	if (instrument == &Piano1)
		return;

	if (reg_get_int("instr.0", 1)) {
		ESP_LOGI(tag, "Selected instrument: Piano1");
		instrument_select(&Piano1);
		return;
	}

select_piano2:
	if (instrument == &Piano2)
		return;

	if (reg_get_int("instr.1", 1)) {
		ESP_LOGI(tag, "Selected instrument: Piano2");
		instrument_select(&Piano2);
		return;
	}

select_extras:
	if (instrument == &Extras)
		return;

	if (reg_get_int("instr.2", 1)) {
		ESP_LOGI(tag, "Selected instrument: Extras");
		instrument_select(&Extras);
		return;
	}

	goto select_piano1;
}


void instrument_press(int key)
{
	instrument->key_press(key);
	instrument->key_release(key);
}
