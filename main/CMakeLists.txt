idf_component_register(
	SRCS
		"main.c"

	INCLUDE_DIRS
		"."

	PRIV_REQUIRES
		soc
		driver
		led_strip
		nvs_flash
)
