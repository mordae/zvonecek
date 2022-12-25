idf_component_register(
	SRCS
		"main.c"
		"synth.c"
		"scene.c"
		"led.c"
		"strings.c"
		"player.c"
		"scene/keyboard.c"
		"scene/learning.c"

	INCLUDE_DIRS
		"."

	PRIV_REQUIRES
		soc
		driver
		led_strip
		nvs_flash
		esp_pm
)
