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


#include "scene.h"
#include "strings.h"
#include "player.h"
#include "led.h"

#include "esp_log.h"
#include "esp_timer.h"

#include <string.h>


static const char *tag = "learning";


const char *const learning_songs[4] = {
	/* Běží liška k táboru */
	"CECEG GGCECED DDCEGEDDE CEGEDDC",

	/* Skákal pes přes oves */
	"GGE GGE GGAGG F FFD FFD FFGFF E",

	/* Kočka leze dírou */
	"CDEFG G A A G  A A G  FFFFE E D D G  FFFFE E D D C",

	/* Pec nám spadla */
	"GEEEGEEEGGAGGFF FDDDFDDDFFGFFEE",
};


static const char *current_song = learning_songs[0];

/* What note to hit next. */
static int next_note = 0;

static int64_t idle_since = 0;


static void advance(void)
{
	while (true) {
		next_note++;

		if (!current_song[next_note])
			next_note = 0;

		if (note_id(current_song[next_note]) < 0)
			continue;

		break;
	}

	ESP_LOGI(tag, "Prompting for %c", current_song[next_note]);
	led_note(note_id(current_song[next_note]));
}


static void on_init(void)
{
}

static void on_top(void)
{
	ESP_LOGI(tag, "Learning scene now on top...");

	ESP_LOGI(tag, "Playing...");

	play_song(current_song, 1);

	next_note = -1;
	advance();
}

static void on_activate(const void *arg)
{
	ESP_LOGI(tag, "Activated Learning scene...");

	current_song = arg;

	ESP_LOGI(tag, "Selected song: %s", current_song);

	if (current_song == learning_songs[0])
		play_song("CCC      ", 2);
	else if (current_song == learning_songs[1])
		play_song("DDD      ", 2);
	else if (current_song == learning_songs[2])
		play_song("EEE      ", 2);
	else if (current_song == learning_songs[3])
		play_song("FFF      ", 2);

	idle_since = esp_timer_get_time();
}

static void on_deactivate(void)
{
	ESP_LOGI(tag, "Deactivated Learning scene...");
	led_note(-1);
}

static unsigned on_idle(unsigned depth)
{
	int64_t now = esp_timer_get_time();

	if ((now - idle_since) > (CONFIG_IDLE_TIMEOUT * 1000 * 1000)) {
		idle_since += CONFIG_IDLE_REPEAT * 1000 * 1000;
		int note = note_id(current_song[next_note]);
		synth_string_pluck(&strings_current[note]);
	}

	return 1000;
}

static bool on_key_pressed(int key)
{
	idle_since = esp_timer_get_time();

	if (key < NUM_STRINGS) {
		synth_string_pluck(&strings_current[key]);

		if (key == note_id(current_song[next_note])) {
			advance();
		}

		return true;
	}

	if (13 == key) {
		if (current_song == learning_songs[0])
			scene_pop();
		else
			scene_replace(&Learning, learning_songs[0]);

		return true;
	}

	if (14 == key) {
		if (current_song == learning_songs[1])
			scene_pop();
		else
			scene_replace(&Learning, learning_songs[1]);

		return true;
	}

	if (15 == key) {
		if (current_song == learning_songs[2])
			scene_pop();
		else
			scene_replace(&Learning, learning_songs[2]);

		return true;
	}

	if (16 == key) {
		if (current_song == learning_songs[3])
			scene_pop();
		else
			scene_replace(&Learning, learning_songs[3]);

		return true;
	}

	return false;
}

static bool on_key_released(int key)
{
	if (key < NUM_STRINGS) {
		synth_string_dampen(&strings_current[key]);
		return true;
	}

	return false;
}


struct scene Learning = {
	.on_init = on_init,
	.on_top = on_top,
	.on_activate = on_activate,
	.on_deactivate = on_deactivate,
	.on_idle = on_idle,
	.on_key_pressed = on_key_pressed,
	.on_key_released = on_key_released,
};
