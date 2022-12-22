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

#include "synth.h"

#include "config.h"

#include "led_strip.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>


static const char *tag = "main";

#define NOTE_COUNT (3 * 12 + 1)
#define KEYBOARD 13

#define NOTE_C4  261.6256
#define NOTE_C4s 277.1826
#define NOTE_D4  293.6648
#define NOTE_D4s 311.1270
#define NOTE_E4  329.6276
#define NOTE_F4  349.2282
#define NOTE_F4s 369.9944
#define NOTE_G4  391.9954
#define NOTE_G4s 415.3047
#define NOTE_A4  440.0000
#define NOTE_A4s 466.1638
#define NOTE_H4  493.8833


static struct synth_string string[NOTE_COUNT] = {
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_C4,
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_C4s,
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_D4,
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_D4s,
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_E4,
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_F4,
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_F4s,
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_G4,
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_G4s,
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_A4,
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_A4s,
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_H4,
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_C4 * 2),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_C4s * 2),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_D4 * 2),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_D4s * 2),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_E4 * 2),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_F4 * 2),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_F4s * 2),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_G4 * 2),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_G4s * 2),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_A4 * 2),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_A4s * 2),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_H4 * 2),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_C4 * 4),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_C4s * 4),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_D4 * 4),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_D4s * 4),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_E4 * 4),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_F4 * 4),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_F4s * 4),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_G4 * 4),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_G4s * 4),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_A4 * 4),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_A4s * 4),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_H4 * 4),
		.feedback = 0.80,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / (NOTE_C4 * 8),
		.feedback = 0.80,
	},
};



/* Mapping of characters to notes for the songs below. */
static const char note_table[] = "CcDdEFfGgAaH";

/* Běží liška k táboru */
static const char song0[] = "CECEG GGCECED DDCEGEDDE CEGEDDC";

/* Skákal pes přes oves */
static const char song1[] = "GGE GGE GGAGG F FFD FFD FFGFF E";

/* Kočka leze dírou */
static const char song2[] = "CDEFG G A A G  A A G  FFFFE E D D G  FFFFE E D D C";


#define BUFFER_SIZE 64
static int16_t buffer[BUFFER_SIZE];

/* To transpose to higher octaves. */
static int transpose = 12;


static led_strip_handle_t led;
static i2s_chan_handle_t snd;


/* These change depending on the volume slider. */
static int16_t max_volume = 16000;
static int16_t note_volume = 11500;


/* Keys, organized to three rows of 6 keys each. */
#define NUM_KEYS 18
static bool keys[NUM_KEYS] = {false};
static bool prev_keys[NUM_KEYS] = {false};


static void playback_task(void *arg)
{
	while (true) {
		for (int i = 0; i < BUFFER_SIZE; i++)
			buffer[i] = 0;

		/*
		 * Add samples from all strings to the buffer.
		 * Most are going to be zeroes.
		 */
		for (int i = 0; i < KEYBOARD; i++)
			synth_string_read(string + transpose + i, buffer, BUFFER_SIZE);

		/*
		 * Duck volume of the whole buffer if it would exceed the
		 * maximum allowed. This is rather crude.
		 */
		int peak = 0;

		for (int i = 0; i < BUFFER_SIZE; i++) {
			if (abs(buffer[i]) > peak)
				peak = abs(buffer[i]);
		}

		if (peak > max_volume) {
			float ratio = (float)max_volume / (float)peak;
			for (int i = 0; i < BUFFER_SIZE; i++)
				buffer[i] *= ratio;
		}

		/*
		 * Our buffer is small, so we should be able to emit it whole.
		 */
		size_t total = BUFFER_SIZE * sizeof(int16_t);
		size_t written = 0;

		ESP_ERROR_CHECK(i2s_channel_write(snd, buffer, total, &written, portMAX_DELAY));
		assert (total == written);
	}
}


static void led_reset()
{
	for (int i = 0; i < 8; i++)
		ESP_ERROR_CHECK(led_strip_set_pixel(led, i, 0, 0, 0));
}


static void led_note(int note)
{
	led_reset();

	if (0 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, 31, 31, 31));
	} else if (1 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, 63, 0, 0));
	} else if (2 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 1, 31, 31, 31));
	} else if (3 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 1, 63, 0, 0));
	} else if (4 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 2, 31, 31, 31));
	} else if (5 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 3, 31, 31, 31));
	} else if (6 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 3, 63, 0, 0));
	} else if (7 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 4, 31, 31, 31));
	} else if (8 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 4, 63, 0, 0));
	} else if (9 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 5, 31, 31, 31));
	} else if (10 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 5, 63, 0, 0));
	} else if (11 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 6, 31, 31, 31));
	} else if (12 == note) {
		ESP_ERROR_CHECK(led_strip_set_pixel(led, 7, 31, 31, 31));
	}

	ESP_ERROR_CHECK(led_strip_refresh(led));
}


static void play_song(const char *song)
{
	for (const char *c = song; *c; c++) {
		if ((*c) == ' ') {
			led_note(-1);
			vTaskDelay(pdMS_TO_TICKS(300));
		} else {
			int note = strchrnul(note_table, *c) - note_table;
			led_note(note);
			synth_string_pluck_shortly(string + transpose + note, note_volume);
			vTaskDelay(pdMS_TO_TICKS(200));
			led_note(-1);
			vTaskDelay(pdMS_TO_TICKS(100));
		}
	}

	led_note(-1);
}


void app_main(void)
{
	ESP_LOGI(tag, "Configure LED...");
	led_strip_config_t config = {
		.strip_gpio_num = CONFIG_LED_GPIO,
		.max_leds = 8,
	};
	ESP_ERROR_CHECK(led_strip_new_rmt_device(&config, &led));

	ESP_LOGI(tag, "Detect volume level...");
	gpio_config_t gpio_vol = {
		.pin_bit_mask = BIT64(CONFIG_VOLUME_GPIO),
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_INPUT,
	};

	ESP_ERROR_CHECK(gpio_config(&gpio_vol));

	if (gpio_get_level(CONFIG_VOLUME_GPIO)) {
		ESP_LOGI(tag, "Volume: high");
	} else {
		ESP_LOGI(tag, "Volume: low");
		max_volume *= 0.5;
		note_volume *= 0.5;
	}

	ESP_LOGI(tag, "Configure keys...");
	gpio_config_t gpio_keys = {
		.pin_bit_mask = BIT64(CONFIG_KEY1_GPIO)
		              | BIT64(CONFIG_KEY2_GPIO)
		              | BIT64(CONFIG_KEY3_GPIO)
		              | BIT64(CONFIG_KEY4_GPIO)
		              | BIT64(CONFIG_KEY5_GPIO)
		              | BIT64(CONFIG_KEY6_GPIO)
		              ,
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_INPUT,
		.pull_down_en = 0,
		.pull_up_en = 1,
	};

	ESP_ERROR_CHECK(gpio_config(&gpio_keys));

	ESP_LOGI(tag, "Configure rows...");
	gpio_config_t gpio_rows = {
		.pin_bit_mask = BIT64(CONFIG_ROW1_GPIO)
		              | BIT64(CONFIG_ROW2_GPIO)
		              | BIT64(CONFIG_ROW3_GPIO)
		              ,
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_OUTPUT_OD,
		.pull_down_en = 0,
		.pull_up_en = 0,
	};

	ESP_ERROR_CHECK(gpio_config(&gpio_rows));

	ESP_LOGI(tag, "Configure i2s output...");
	i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
	chan_cfg.auto_clear = true;
	ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &snd, NULL));

	i2s_std_config_t std_cfg = {
		.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
		.slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
		.gpio_cfg = {
			.mclk = I2S_GPIO_UNUSED,
			.bclk = GPIO_NUM_33,
			.ws = GPIO_NUM_32,
			.dout = GPIO_NUM_25,
			.din = I2S_GPIO_UNUSED,
		},
	};

	ESP_ERROR_CHECK(i2s_channel_init_std_mode(snd, &std_cfg));
	ESP_ERROR_CHECK(i2s_channel_enable(snd));

	ESP_LOGI(tag, "Start the playback task...");
	xTaskCreate(playback_task, "playback", 4096, NULL, 0, NULL);

	ESP_LOGI(tag, "Start demonstration...");
	ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW1_GPIO, 1));
	ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW2_GPIO, 1));
	ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW3_GPIO, 1));

	led_reset();
	ESP_ERROR_CHECK(led_strip_refresh(led));

	while (1) {
		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW1_GPIO, 0));
		vTaskDelay(1);

		keys[13] = !gpio_get_level(CONFIG_KEY1_GPIO);
		keys[14] = !gpio_get_level(CONFIG_KEY2_GPIO);
		keys[15] = !gpio_get_level(CONFIG_KEY3_GPIO);
		keys[16] = !gpio_get_level(CONFIG_KEY4_GPIO);
		keys[17] = !gpio_get_level(CONFIG_KEY5_GPIO);
		keys[ 3] = !gpio_get_level(CONFIG_KEY6_GPIO);

		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW1_GPIO, 1));
		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW2_GPIO, 0));
		vTaskDelay(1);

		keys[ 2] = !gpio_get_level(CONFIG_KEY1_GPIO);
		keys[ 8] = !gpio_get_level(CONFIG_KEY2_GPIO);
		keys[ 6] = !gpio_get_level(CONFIG_KEY3_GPIO);
		keys[10] = !gpio_get_level(CONFIG_KEY4_GPIO);
		keys[ 1] = !gpio_get_level(CONFIG_KEY5_GPIO);
		keys[ 0] = !gpio_get_level(CONFIG_KEY6_GPIO);

		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW2_GPIO, 1));
		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW3_GPIO, 0));
		vTaskDelay(1);

		keys[ 4] = !gpio_get_level(CONFIG_KEY1_GPIO);
		keys[ 5] = !gpio_get_level(CONFIG_KEY2_GPIO);
		keys[ 7] = !gpio_get_level(CONFIG_KEY3_GPIO);
		keys[ 9] = !gpio_get_level(CONFIG_KEY4_GPIO);
		keys[11] = !gpio_get_level(CONFIG_KEY5_GPIO);
		keys[12] = !gpio_get_level(CONFIG_KEY6_GPIO);

		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW3_GPIO, 1));

		int total_pressed = 0;

		for (int i = 0; i < NUM_KEYS; i++) {
			if (keys[i] && !prev_keys[i]) {
				ESP_LOGI(tag, "Key %i down", i);
			} else if (!keys[i] && prev_keys[i]) {
				ESP_LOGI(tag, "Key %i up", i);
			}
			total_pressed += keys[i];
		}

		for (int i = 0; i < KEYBOARD; i++) {
			if (keys[i] && !prev_keys[i]) {
				ESP_LOGI(tag, "Pluck string %i", transpose + i);
				synth_string_pluck(string + transpose + i, note_volume);
			}
			else if (!keys[i] && prev_keys[i]) {
				ESP_LOGI(tag, "Dampen string %i", transpose + i);
				string[transpose + i].decay = 0.985;
			}
		}

		if (keys[13] && !prev_keys[13] && 1 == total_pressed) {
			play_song(song0);
		}

		if (keys[14] && !prev_keys[14] && 1 == total_pressed) {
			play_song(song1);
		}

		if (keys[15] && !prev_keys[15] && 1 == total_pressed) {
			play_song(song2);
		}

		if (keys[16] && !prev_keys[16]) {
			/* free */
		}

		if (keys[17] && 2 == total_pressed) {
			if (keys[0] && !prev_keys[0]) {
				for (int i = 0; i < NUM_KEYS; i++) {
					string[i].decay -= 0.001;
				}
				ESP_LOGI(tag, "decay = %f", string[0].decay);
			} else if (keys[2] && !prev_keys[2]) {
				for (int i = 0; i < NUM_KEYS; i++) {
					string[i].decay += 0.001;
				}
				ESP_LOGI(tag, "decay = %f", string[0].decay);
			} else if (keys[4] && !prev_keys[4]) {
				for (int i = 0; i < NUM_KEYS; i++) {
					string[i].feedback -= 0.01;
				}
				ESP_LOGI(tag, "feedback = %f", string[0].feedback);
			} else if (keys[5] && !prev_keys[5]) {
				for (int i = 0; i < NUM_KEYS; i++) {
					string[i].feedback += 0.01;
				}
				ESP_LOGI(tag, "feedback = %f", string[0].feedback);
			} else if (keys[7] && !prev_keys[7]) {
				if (transpose == 0)
					transpose = 12;
				else if (transpose == 12)
					transpose = 24;
				else if (transpose == 24)
					transpose = 0;
				ESP_LOGI(tag, "transpose = %i", transpose);
			}
		}

		for (int i = 0; i < NUM_KEYS; i++)
			prev_keys[i] = keys[i];

		vTaskDelay(pdMS_TO_TICKS(50));
	}
}
