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
#include "instrument.h"
#include "registry.h"

#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_pm.h"
#include "esp_rom_sys.h"

#include <math.h>
#include <stdlib.h>


static const char *tag = "main";


#define BUFFER_SIZE (CONFIG_SAMPLE_FREQ / 100)
static float buffer[BUFFER_SIZE];
static int16_t buffer_i16[BUFFER_SIZE];

static i2s_chan_handle_t snd;

/* Absolute maximum volume. */
static float max_volume = INT16_MAX;

/* (global) Default volume.
 * Multiple tones can combine and exceed even the maximum volume. */
float volume = 0.25;

/* Quiet mode. Depends on the position of the power switch. */
static bool quiet = false;


/* Keys, organized to three rows of 6 keys each. */
#define NUM_KEYS 18
static bool keys[NUM_KEYS] = {false};
static bool prev_keys[NUM_KEYS] = {false};


static void playback_task(void *arg)
{
	static bool enabled = false;
	static int idle = 0;

	while (true) {
		for (int i = 0; i < BUFFER_SIZE; i++)
			buffer[i] = 0;

		/*
		 * Add samples from all strings to the buffer.
		 * Most are going to be zeroes.
		 */
		instrument->read(buffer, BUFFER_SIZE);

		/* Total value of samples in the last buffer. When it hits zero,
		 * we do not output anything but rather disable the amplifier. */
		size_t level = 0;

		/*
		 * Our buffer is small, so we should be able to emit it whole.
		 */
		size_t total = BUFFER_SIZE * sizeof(int16_t);
		size_t written = 0;

		/* Take quiet setting into account. */
		float normal_volume = volume * (quiet ? 0.5 : 1.0);

		for (int i = 0; i < BUFFER_SIZE; i++) {
			float sample = buffer[i] * normal_volume;

			if (sample > max_volume)
				sample = max_volume;

			buffer_i16[i] = sample;
			level += sample;
		}

		if (level && !enabled) {
			ESP_LOGI(tag, "Enable audio...");
			ESP_ERROR_CHECK(i2s_channel_enable(snd));
			enabled = true;
			idle = 0;
		} else if (!level && enabled && (++idle >= 100)) {
			ESP_LOGI(tag, "Disable audio...");
			ESP_ERROR_CHECK(i2s_channel_disable(snd));
			enabled = false;
			vTaskDelay(pdMS_TO_TICKS(BUFFER_SIZE * 1000 / CONFIG_SAMPLE_FREQ));
			continue;
		} else if (!level && !enabled) {
			vTaskDelay(pdMS_TO_TICKS(BUFFER_SIZE * 1000 / CONFIG_SAMPLE_FREQ));
			continue;
		}

		ESP_ERROR_CHECK(i2s_channel_write(snd, buffer_i16, total, &written, portMAX_DELAY));
		assert (total == written);
	}
}


void app_main(void)
{
	ESP_LOGI(tag, "Configure power management...");
	esp_pm_config_esp32_t pm_cfg = {
		.min_freq_mhz = 240,
		.max_freq_mhz = 240,
		.light_sleep_enable = 1,
	};
	ESP_ERROR_CHECK(esp_pm_configure(&pm_cfg));

	reg_init();

	ESP_LOGI(tag, "Configure LED...");
	led_init(CONFIG_LED_GPIO);

	ESP_LOGI(tag, "Detect volume level...");
	gpio_config_t gpio_vol = {
		.pin_bit_mask = BIT64(CONFIG_VOLUME_GPIO),
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_INPUT,
	};

	ESP_ERROR_CHECK(gpio_config(&gpio_vol));

	/* Restore the saved volume. */
	volume = reg_get_int("volume", volume) / 1000.0;

	if (gpio_get_level(CONFIG_VOLUME_GPIO)) {
		ESP_LOGI(tag, "Volume: loud");
		quiet = false;
	} else {
		ESP_LOGI(tag, "Volume: quiet");
		quiet = true;
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
		.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(CONFIG_SAMPLE_FREQ),
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
		esp_rom_delay_us(5);

		keys[13] = !gpio_get_level(CONFIG_KEY1_GPIO);
		keys[14] = !gpio_get_level(CONFIG_KEY2_GPIO);
		keys[15] = !gpio_get_level(CONFIG_KEY3_GPIO);
		keys[16] = !gpio_get_level(CONFIG_KEY4_GPIO);
		keys[17] = !gpio_get_level(CONFIG_KEY5_GPIO);
		keys[ 3] = !gpio_get_level(CONFIG_KEY6_GPIO);

		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW1_GPIO, 1));
		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW2_GPIO, 0));
		esp_rom_delay_us(5);

		keys[ 2] = !gpio_get_level(CONFIG_KEY1_GPIO);
		keys[ 8] = !gpio_get_level(CONFIG_KEY2_GPIO);
		keys[ 6] = !gpio_get_level(CONFIG_KEY3_GPIO);
		keys[10] = !gpio_get_level(CONFIG_KEY4_GPIO);
		keys[ 1] = !gpio_get_level(CONFIG_KEY5_GPIO);
		keys[ 0] = !gpio_get_level(CONFIG_KEY6_GPIO);

		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW2_GPIO, 1));
		ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW3_GPIO, 0));
		esp_rom_delay_us(5);

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
			if (total_pressed > 2)
				continue;

			if (keys[i] && !prev_keys[i]) {
				(void)scene_handle_key_pressed(i);
			} else if (!keys[i] && prev_keys[i]) {
				(void)scene_handle_key_released(i);
			}
		}

		for (int i = 0; i < NUM_KEYS; i++)
			prev_keys[i] = keys[i];

		unsigned sleep = scene_idle(50);
		vTaskDelay(pdMS_TO_TICKS(sleep));
	}
}
