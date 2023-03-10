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

#include "player.h"
#include "instrument.h"
#include "led.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>


static const char note_table[] = "CcDdEFfGgAaH+";


void play_song(const char *song, float tempo)
{
	for (const char *c = song; *c; c++) {
		int id = note_id(*c);

		if (-1 == id) {
			led_note(-1);
			vTaskDelay(pdMS_TO_TICKS(300 / tempo));
		} else {
			led_note(id);
			instrument_press(id);
			vTaskDelay(pdMS_TO_TICKS(200 / tempo));
			led_note(-1);
			vTaskDelay(pdMS_TO_TICKS(100 / tempo));
		}
	}

	led_note(-1);
}


int note_id(char note)
{
	if (' ' == note)
		return -1;

	const char *ptr = strchr(note_table, note);

	if (NULL == ptr)
		return -1;

	return ptr - note_table;
}
