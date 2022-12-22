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


#define CONFIG_LED_GPIO 16
#define CONFIG_VOLUME_GPIO 18

#define CONFIG_ROW1_GPIO 26
#define CONFIG_ROW2_GPIO 4
#define CONFIG_ROW3_GPIO 17

#define CONFIG_KEY1_GPIO 27
#define CONFIG_KEY2_GPIO 14
#define CONFIG_KEY3_GPIO 12
#define CONFIG_KEY4_GPIO 13
#define CONFIG_KEY5_GPIO 15
#define CONFIG_KEY6_GPIO 2

#define CONFIG_SAMPLE_FREQ 48000

#define NOTE_COUNT 13
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
#define NOTE_B4  493.8833
#define NOTE_C5  523.2511


static struct synth_string string[NOTE_COUNT] = {
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_C4,
		.decay = 0.99,
		.feedback = 0.90,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_C4s,
		.decay = 0.99,
		.feedback = 0.90,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_D4,
		.decay = 0.99,
		.feedback = 0.90,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_D4s,
		.decay = 0.99,
		.feedback = 0.90,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_E4,
		.decay = 0.99,
		.feedback = 0.90,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_F4,
		.decay = 0.99,
		.feedback = 0.90,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_F4s,
		.decay = 0.99,
		.feedback = 0.90,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_G4,
		.decay = 0.99,
		.feedback = 0.90,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_G4s,
		.decay = 0.99,
		.feedback = 0.90,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_A4,
		.decay = 0.99,
		.feedback = 0.90,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_A4s,
		.decay = 0.99,
		.feedback = 0.90,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_B4,
		.decay = 0.99,
		.feedback = 0.90,
	},
	{
		.delay = CONFIG_SAMPLE_FREQ / NOTE_C5,
		.decay = 0.99,
		.feedback = 0.90,
	},
};


/* Běží liška k táboru */
static const char song0[] = "CECEG GGCECED DDCEGEDDE CEGEDDC";


#define BUFFER_SIZE 64
static int16_t buffer[BUFFER_SIZE];


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
		for (int i = 0; i < NOTE_COUNT; i++)
			synth_string_read(string + i, buffer, BUFFER_SIZE);

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
		.mode = GPIO_MODE_OUTPUT,
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

	ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, 3, 3, 3));
	ESP_ERROR_CHECK(led_strip_set_pixel(led, 1, 3, 3, 3));
	ESP_ERROR_CHECK(led_strip_set_pixel(led, 2, 3, 3, 3));
	ESP_ERROR_CHECK(led_strip_set_pixel(led, 3, 3, 3, 3));
	ESP_ERROR_CHECK(led_strip_set_pixel(led, 4, 3, 3, 3));
	ESP_ERROR_CHECK(led_strip_set_pixel(led, 5, 3, 3, 3));
	ESP_ERROR_CHECK(led_strip_set_pixel(led, 6, 3, 3, 3));
	ESP_ERROR_CHECK(led_strip_set_pixel(led, 7, 3, 3, 3));
	ESP_ERROR_CHECK(led_strip_refresh(led));

	while (1) {
		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW1_GPIO, 0));
		keys[13] = !gpio_get_level(CONFIG_KEY1_GPIO);
		keys[14] = !gpio_get_level(CONFIG_KEY2_GPIO);
		keys[15] = !gpio_get_level(CONFIG_KEY3_GPIO);
		keys[16] = !gpio_get_level(CONFIG_KEY4_GPIO);
		keys[17] = !gpio_get_level(CONFIG_KEY5_GPIO);
		keys[ 3] = !gpio_get_level(CONFIG_KEY6_GPIO);
		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW1_GPIO, 1));

		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW2_GPIO, 0));
		keys[ 2] = !gpio_get_level(CONFIG_KEY1_GPIO);
		keys[ 8] = !gpio_get_level(CONFIG_KEY2_GPIO);
		keys[ 6] = !gpio_get_level(CONFIG_KEY3_GPIO);
		keys[10] = !gpio_get_level(CONFIG_KEY4_GPIO);
		keys[ 1] = !gpio_get_level(CONFIG_KEY5_GPIO);
		keys[ 0] = !gpio_get_level(CONFIG_KEY6_GPIO);
		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW2_GPIO, 1));

		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW3_GPIO, 0));
		keys[ 4] = !gpio_get_level(CONFIG_KEY1_GPIO);
		keys[ 5] = !gpio_get_level(CONFIG_KEY2_GPIO);
		keys[ 7] = !gpio_get_level(CONFIG_KEY3_GPIO);
		keys[ 9] = !gpio_get_level(CONFIG_KEY4_GPIO);
		keys[11] = !gpio_get_level(CONFIG_KEY5_GPIO);
		keys[12] = !gpio_get_level(CONFIG_KEY6_GPIO);
		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW3_GPIO, 1));

		for (int i = 0; i < NOTE_COUNT; i++) {
			if (keys[i] && !prev_keys[i]) {
				ESP_LOGI(tag, "Pluck string %i", i);
				synth_string_pluck(string + i, note_volume);
			}
		}

		if (keys[13] && !prev_keys[13]) {
			const char notes[] = "CcDdEFfGgAaH";
			for (const char *c = song0; *c; c++) {
				if ((*c) != ' ') {
					int note = strchrnul(notes, *c) - notes;
					synth_string_pluck(string + note, note_volume);
				}
				vTaskDelay(pdMS_TO_TICKS(333));
			}
		}

		if (keys[14] && !prev_keys[14]) {
			for (int i = 0; i < NUM_KEYS; i++) {
				string[i].decay -= 0.001;
			}
			ESP_LOGI(tag, "decay = %f", string[0].decay);
		}

		if (keys[15] && !prev_keys[15]) {
			for (int i = 0; i < NUM_KEYS; i++) {
				string[i].decay += 0.001;
			}
			ESP_LOGI(tag, "decay = %f", string[0].decay);
		}

		if (keys[16] && !prev_keys[16]) {
			for (int i = 0; i < NUM_KEYS; i++) {
				string[i].feedback -= 0.01;
			}
			ESP_LOGI(tag, "feedback = %f", string[0].feedback);
		}

		if (keys[17] && !prev_keys[17]) {
			for (int i = 0; i < NUM_KEYS; i++) {
				string[i].feedback += 0.01;
			}
			ESP_LOGI(tag, "feedback = %f", string[0].feedback);
		}

		for (int i = 0; i < NUM_KEYS; i++)
			prev_keys[i] = keys[i];

		vTaskDelay(pdMS_TO_TICKS(50));
	}
}
