idf_component_register(
	SRCS
		"main.c"
		"synth.c"
		"scene.c"
		"led.c"
		"strings.c"
		"player.c"
		"registry.c"
		"instrument.c"
		"scene/keyboard.c"
		"scene/learning.c"
		"scene/menu.c"

	INCLUDE_DIRS
		"."

	PRIV_REQUIRES
		soc
		driver
		led_strip
		nvs_flash
		esp_pm
		esp_timer
)
