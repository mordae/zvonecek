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


static const char *tag = "keyboard";

/* Song to play when entering the scene. */
static const char intro_song[] = "CDEFGAH+";

/* Song to play when left idle for too long. */
static const char idle_song[] = "gGgGFC CFAAG F";

/* How many μs have it been since last input? */
static int64_t idle_since = 0;


static void on_init(void)
{
}

static void on_top(void)
{
	ESP_LOGI(tag, "Keyboard scene now on top...");
	play_song(intro_song, 4);
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
	int64_t now = esp_timer_get_time();

	if (depth > 0) {
		idle_since = now;
		return 1000;
	}

	if ((now - idle_since) > (CONFIG_IDLE_TIMEOUT * 1000 * 1000)) {
		idle_since += CONFIG_IDLE_REPEAT * 1000 * 1000;
		play_song(idle_song, 2);
	}

	return 1000;
}

static bool on_key_pressed(int key)
{
	idle_since = esp_timer_get_time();

	if (key < NUM_STRINGS) {
		synth_string_pluck(&strings_current[key]);
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
			strings_current = strings_piano1;

		synth_string_pluck(&strings_current[0]);

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
