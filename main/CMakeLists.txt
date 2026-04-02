idf_component_register(
    SRCS
        "main.c"
        "motor.c"
        "hall.c"
        "leds.c"
        "homing.c"
        "idle.c"
        "blink.c"
       
        "next_state.c"
        "neon_flicker.c"
    INCLUDE_DIRS "."
    REQUIRES
        freertos
        esp_timer
        esp_adc
        led_strip
        driver
        esp_hw_support
)
