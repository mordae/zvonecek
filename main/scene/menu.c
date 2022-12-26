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


static const char *tag = "menu";

/* Song to play when entering the scene. */
static const char intro_song[] = "+HA GAH";

/* Colors of individual LEDs for batch updates. */
static struct led_color leds[8] = {
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0, 255,   0},
	{255,   0,   0},
};


static void on_init(void)
{
}

static void on_top(void)
{
	ESP_LOGI(tag, "Menu scene now on top...");
	play_song(intro_song, 2);
	led_set(leds);
}

static void on_activate(const void *arg)
{
	ESP_LOGI(tag, "Activated Menu scene...");
}

static void on_deactivate(void)
{
	ESP_LOGI(tag, "Deactivated Menu scene...");
	led_reset();
}

static unsigned on_idle(unsigned depth)
{
	return 1000;
}

static bool on_key_pressed(int key)
{
	if (11 == key) {
		play_song("H+  ", 2);
		scene_pop();
		return true;
	}

	if (12 == key) {
		play_song("AE  ", 2);
		scene_pop();
		return true;
	}

	return false;
}

static bool on_key_released(int key)
{
	return false;
}


struct scene Menu = {
	.on_init = on_init,
	.on_top = on_top,
	.on_activate = on_activate,
	.on_deactivate = on_deactivate,
	.on_idle = on_idle,
	.on_key_pressed = on_key_pressed,
	.on_key_released = on_key_released,
};
