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
#include "main.h"
#include "led.h"

#include "esp_log.h"


static const char *tag = "keyboard";


//static const char intro_song[] = "gGgGFC CFAAG F";
static const char intro_song[] = "CDEFGAH+";


static void on_init(void)
{
}

static void on_top(void)
{
	ESP_LOGI(tag, "Keyboard scene now on top...");
	play_song(intro_song, 4, base_volume);
	led_backlight();
}

static void on_activate(const void *arg)
{
	ESP_LOGI(tag, "Activated Keyboard scene...");
}

static void on_deactivate(void)
{
	ESP_LOGI(tag, "Deactivated Keyboard scene...");
	led_reset();
}

static unsigned on_idle(unsigned depth)
{
	return 1000;
}

static bool on_key_pressed(int key)
{
	if (key < NUM_STRINGS) {
		synth_string_pluck(&strings_current[key], base_volume);
		return true;
	}

	if (13 == key) {
		scene_push(&Learning, learning_songs[0]);
		return true;
	}

	if (14 == key) {
		scene_push(&Learning, learning_songs[1]);
		return true;
	}

	if (15 == key) {
		scene_push(&Learning, learning_songs[2]);
		return true;
	}

	if (16 == key) {
		scene_push(&Learning, learning_songs[3]);
		return true;
	}

	if (17 == key) {
		if (strings_current == strings_piano1)
			strings_current = strings_piano2;
		else if (strings_current == strings_piano2)
			strings_current = strings_guitar;
		else if (strings_current == strings_guitar)
			strings_current = strings_piano1;

		synth_string_pluck(&strings_current[0], base_volume);

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


struct scene Keyboard = {
	.on_init = on_init,
	.on_top = on_top,
	.on_activate = on_activate,
	.on_deactivate = on_deactivate,
	.on_idle = on_idle,
	.on_key_pressed = on_key_pressed,
	.on_key_released = on_key_released,
};