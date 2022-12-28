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


struct instrument Piano1 = {
	.enable = piano1_enable,
	.key_press = piano1_key_press,
	.key_release = piano1_key_release,
	.read = piano1_read,
};

struct instrument Piano2 = {
	.enable = piano2_enable,
	.key_press = piano2_key_press,
	.key_release = piano2_key_release,
	.read = piano2_read,
};

struct instrument *instrument = &Piano2;


void instrument_select(struct instrument *inst)
{
	instrument = inst;
	instrument->enable();
}


void instrument_press(int key)
{
	instrument->key_press(key);
	instrument->key_release(key);
}


void instrument_next(void)
{
	if (instrument == &Piano1) {
		if (reg_get_int("instr.1", 1)) {
			ESP_LOGI(tag, "Selected instrument: Piano2");
			instrument_select(&Piano2);
			return;
		}

		return;
	}

	if (instrument == &Piano2) {
		if (reg_get_int("instr.0", 1)) {
			ESP_LOGI(tag, "Selected instrument: Piano1");
			instrument_select(&Piano1);
			return;
		}

		return;
	}
}
