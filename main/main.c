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
#include "scene.h"
#include "synth.h"
#include "player.h"
#include "strings.h"

#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"

#include <math.h>
#include <stdlib.h>


static const char *tag = "main";


#define BUFFER_SIZE 64
static int16_t buffer[BUFFER_SIZE];

static i2s_chan_handle_t snd;

/* Depends on the volume slide switch. */
int16_t max_volume = 16000;
int16_t base_volume = 11500;


/* Keys, organized to three rows of 6 keys each. */
#define NUM_NOTE_KEYS 13
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
		for (int i = 0; i < NUM_NOTE_KEYS; i++)
			synth_string_read(&strings_current[i], buffer, BUFFER_SIZE);

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
	led_init(CONFIG_LED_GPIO);

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
		base_volume *= 0.5;
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

	ESP_LOGI(tag, "Seed the random number generator...");
	srand(esp_random());

	ESP_LOGI(tag, "Start the playback task...");
	xTaskCreate(playback_task, "playback", 4096, NULL, 0, NULL);

	ESP_LOGI(tag, "Initialize scenes...");
	Keyboard.on_init();
	Learning.on_init();

	ESP_LOGI(tag, "Start the Keyboard scene...");
	scene_push(&Keyboard, NULL);

	ESP_LOGI(tag, "Begin scanning keys...");
	ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW1_GPIO, 1));
	ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW2_GPIO, 1));
	ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW3_GPIO, 1));

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

		for (int i = 0; i < NUM_KEYS; i++) {
			if (keys[i] && !prev_keys[i]) {
				if ((i < NUM_NOTE_KEYS) || (1 == total_pressed))
					(void)scene_handle_key_pressed(i);
			}
			else if (!keys[i] && prev_keys[i]) {
				(void)scene_handle_key_released(i);
			}
		}

		for (int i = 0; i < NUM_KEYS; i++)
			prev_keys[i] = keys[i];

		unsigned sleep = scene_idle(50);
		vTaskDelay(pdMS_TO_TICKS(sleep));
	}
}
