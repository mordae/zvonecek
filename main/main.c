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


static led_strip_handle_t led;
static i2s_chan_handle_t snd;


/* Keys, organized to three rows of 6 keys each. */
static bool keys[18] = {false};


#define CONFIG_SAMPLE_FREQ 48000

#define CONFIG_LED_GPIO 16

#define CONFIG_ROW1_GPIO 26
#define CONFIG_ROW2_GPIO 4
#define CONFIG_ROW3_GPIO 17

#define CONFIG_KEY1_GPIO 27
#define CONFIG_KEY2_GPIO 14
#define CONFIG_KEY3_GPIO 12
#define CONFIG_KEY4_GPIO 13
#define CONFIG_KEY5_GPIO 15
#define CONFIG_KEY6_GPIO 2


#define BUFFER_SIZE (CONFIG_SAMPLE_FREQ)

static int16_t buffer[BUFFER_SIZE];

static int transpose = 2;


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


static void play_tone(float freq, float hold)
{
	int samples = CONFIG_SAMPLE_FREQ / freq;
	size_t written = 0;

	assert (samples <= BUFFER_SIZE);

	float volume = 0.35;

	if (freq < 440.0) {
		volume = 2.71375e-6 * freq * freq - 0.0026774 * freq + 1.00267;
	} else if (freq > 1760) {
		volume = -4.83459e-7 * freq * freq + 0.00141232 * freq - 0.638116;
	}

	/* Calculate pure sine wave. */
	for (int i = 0; i < samples; i++)
		buffer[i] = INT16_MAX * volume * sinf(i * M_PI * 2 / samples);

	/* Emit it a couple of times. For at least <hold> long. */
	for (int i = 0; i < hold * CONFIG_SAMPLE_FREQ; i += samples)
		i2s_channel_write(snd, buffer, samples * 2, &written, pdMS_TO_TICKS(1000));
}


static void play_noise(float hold)
{
	int samples = CONFIG_SAMPLE_FREQ * hold;
	size_t written = 0;

	assert (samples <= BUFFER_SIZE);

	for (int i = 0; i < samples; i++)
		buffer[i] = 256 * rand() / RAND_MAX;

	i2s_channel_write(snd, buffer, samples * 2, &written, pdMS_TO_TICKS(1000));
}



static float play_next = 0.0;

static void playback_task(void *arg)
{
	while (true) {
		if (play_next > 0.0) {
			play_tone(play_next, 0.1);
		} else {
			play_noise(0.1);
		}
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

		for (int k = 0; k < 18; k++) {
			if (keys[k]) {
				ESP_LOGI(tag, "key%i down", k);
			}
		}

		play_next = 0.0;

		if (keys[0])
			play_next = NOTE_C4 * transpose;

		if (keys[1])
			play_next = NOTE_C4s * transpose;

		if (keys[2])
			play_next = NOTE_D4 * transpose;

		if (keys[3])
			play_next = NOTE_D4s * transpose;

		if (keys[4])
			play_next = NOTE_E4 * transpose;

		if (keys[5])
			play_next = NOTE_F4 * transpose;

		if (keys[6])
			play_next = NOTE_F4s * transpose;

		if (keys[7])
			play_next = NOTE_G4 * transpose;

		if (keys[8])
			play_next = NOTE_G4s * transpose;

		if (keys[9])
			play_next = NOTE_A4 * transpose;

		if (keys[10])
			play_next = NOTE_A4s * transpose;

		if (keys[11])
			play_next = NOTE_B4 * transpose;

		if (keys[12])
			play_next = NOTE_C5 * transpose;

		if (keys[13]) {
			// nothing yet
		}

		if (keys[14]) {
			// nothing yet
		}

		if (keys[15]) {
			if (1 == transpose)
				transpose = 2;
			else if (2 == transpose)
				transpose = 4;
			else if (4 == transpose)
				transpose = 1;

			play_next = NOTE_C4 * transpose;
			vTaskDelay(pdMS_TO_TICKS(300));
			play_next = 0.0;
		}

		if (keys[16]) {
			// nothing yet
		}

		if (keys[17]) {
			// nothing yet
		}

		vTaskDelay(pdMS_TO_TICKS(100));
	}
}
