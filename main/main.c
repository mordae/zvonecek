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


static const char *tag = "main";


static led_strip_handle_t led;
static i2s_chan_handle_t snd;


#define CONFIG_LED_GPIO 16

#define CONFIG_ROW1_GPIO 26
#define CONFIG_ROW2_GPIO 4
#define CONFIG_ROW3_GPIO 17

#define CONFIG_KEY1_GPIO 26
#define CONFIG_KEY2_GPIO 14
#define CONFIG_KEY3_GPIO 12
#define CONFIG_KEY4_GPIO 13
#define CONFIG_KEY5_GPIO 15
#define CONFIG_KEY6_GPIO 2


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

	ESP_LOGI(tag, "Start demonstration...");
	ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW1_GPIO, 1));
	ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW2_GPIO, 0));
	ESP_ERROR_CHECK(gpio_set_level(CONFIG_ROW3_GPIO, 1));

	while (1) {
		if (!gpio_get_level(CONFIG_KEY6_GPIO)) {
			ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, 15, 0, 0));
			ESP_ERROR_CHECK(led_strip_refresh(led));

			int samples = 48000 / 440;
			uint16_t buf[samples];
			size_t written = 0;

			/* Calculate pure sine wave. */
			for (int i = 0; i < samples; i++)
				buf[i] = 4096 * sinf(i * M_PI * 2 / samples);

			/* Emit it couple of times. */
			for (int i = 0; i < 1000; i++)
				i2s_channel_write(snd, buf, sizeof(buf), &written, pdMS_TO_TICKS(1000));

			/* Clear it to silence. */
			for (int i = 0; i < samples; i++)
				buf[i] = 0;

			ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, 0, 0, 7));
			ESP_ERROR_CHECK(led_strip_refresh(led));
		} else {
			ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, 3, 3, 3));
			ESP_ERROR_CHECK(led_strip_refresh(led));
		}

		vTaskDelay(pdMS_TO_TICKS(100));
	}
}
