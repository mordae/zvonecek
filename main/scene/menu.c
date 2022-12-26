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
#include "registry.h"
#include "led.h"

#include "esp_log.h"
#include "esp_timer.h"


static const char *tag = "menu";

/* Song to play when entering the scene. */
static const char intro_song[] = "cdfga";

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

/* Menu items. */
#define MENU_SIZE 2
static int menu[MENU_SIZE] = {1, -1};

/* From main.c. */
extern float volume;

/* Copy of the volume before menu. */
static float initial_volume;


static void on_init(void)
{
}

static void on_top(void)
{
	ESP_LOGI(tag, "Menu scene now on top...");
	initial_volume = volume;
	strings_current = strings_piano2;
	play_song(intro_song, 2);
	led_reset();

	menu[0] = reg_get_int("instr.0", 1);
	menu[1] = -1;
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
	for (int i = 0; i < MENU_SIZE; i++) {
		leds[i].r = leds[i].g = leds[i].b = 0;

		if (menu[i] < 0) {
			leds[i].b = volume * 255;
		} else if (menu[i]) {
			leds[i].g = 255;
		} else {
			leds[i].r = 255;
		}
	}

	led_set(leds);

	return 1000;
}

static bool on_key_pressed(int key)
{
	if (0 == key) {
		menu[0] = !menu[0];
		return true;
	}

	if (1 == key) {
		volume = volume < 0.110 ? 0.100 : volume - 0.010;
		synth_string_pluck_shortly(&strings_current[0]);
		ESP_LOGI(tag, "Volume: %f", volume);
		return true;
	}

	if (3 == key) {
		volume = volume > 0.990 ? 1.000 : volume + 0.010;
		synth_string_pluck_shortly(&strings_current[0]);
		ESP_LOGI(tag, "Volume: %f", volume);
		return true;
	}

	if (11 == key) {
		ESP_LOGI(tag, "Saving settings...");

		reg_set_int("instr.0", menu[0]);
		reg_set_int("instr.1", 1);

		reg_set_int("volume", volume * 1000);

		play_song("H+  ", 2);
		scene_pop();
		return true;
	}

	if (12 == key) {
		volume = initial_volume;
		ESP_LOGI(tag, "Volume: %f", volume);

		play_song("AE  ", 2);
		scene_pop();
		return true;
	}

	/* Do not let the keys fall through. */
	return true;
}

static bool on_key_released(int key)
{
	/* Do not let the keys fall through. */
	return true;
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
