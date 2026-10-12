#pragma once

#include "freertos/FreeRTOS.h"

typedef enum {
    UI_EVENT_NONE = 0,
    UI_EVENT_SHORT_PRESS,
    UI_EVENT_DOUBLE_PRESS,
    UI_EVENT_LONG_PRESS,
} ui_event_t;

typedef enum {
    LED_OFF = 0,
    LED_GREEN_SOLID,
    LED_BLUE_PULSE,
    LED_WHITE_SOLID,
    LED_RED_BLINK,
} led_state_t;

typedef enum {
    HAPTIC_SHORT = 0,
    HAPTIC_DOUBLE,
    HAPTIC_LONG,
} haptic_pattern_t;

void ui_init(void);
ui_event_t ui_wait_for_event(TickType_t timeout);
void ui_set_led(led_state_t state);
void ui_haptic(haptic_pattern_t pattern);
