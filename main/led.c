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


#include "led.h"

#include "led_strip.h"
#include "driver/gpio.h"
#include "esp_log.h"


#define FULL 255
#define HALF 127
#define TINT  63
#define BACK  15


static led_strip_handle_t led;


void led_init(int gpio)
{
	led_strip_config_t config = {
		.strip_gpio_num = gpio,
		.max_leds = 8,
	};
	ESP_ERROR_CHECK(led_strip_new_rmt_device(&config, &led));

	led_strip_suspend(led);
	led_reset();
}


static void refresh()
{
	led_strip_resume(led);
	ESP_ERROR_CHECK(led_strip_refresh(led));
	led_strip_suspend(led);
}


void led_reset(void)
{
	for (int i = 0; i < 8; i++)
		ESP_ERROR_CHECK(led_strip_set_pixel(led, i, 0, 0, 0));

	refresh();
}


void led_note(int note)
{
	for (int i = 0; i < 8; i++)
		ESP_ERROR_CHECK(led_strip_set_pixel(led, i, 0, 0, 0));

	if (0 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, HALF, HALF, HALF));
	} else if (1 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, FULL, 0, TINT));
	} else if (2 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 1, HALF, HALF, HALF));
	} else if (3 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 1, FULL, 0, TINT));
	} else if (4 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 2, HALF, HALF, HALF));
	} else if (5 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 3, HALF, HALF, HALF));
	} else if (6 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 3, FULL, 0, TINT));
	} else if (7 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 4, HALF, HALF, HALF));
	} else if (8 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 4, FULL, 0, TINT));
	} else if (9 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 5, HALF, HALF, HALF));
	} else if (10 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 5, FULL, 0, TINT));
	} else if (11 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 6, HALF, HALF, HALF));
	} else if (12 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 7, HALF, HALF, HALF));
	}

	refresh();
}


void led_backlight(void)
{
	for (int i = 0; i < 8; i++)
		ESP_ERROR_CHECK(led_strip_set_pixel(led, i, BACK, BACK, BACK));

	refresh();
}
