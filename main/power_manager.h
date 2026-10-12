#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TRIGGER_NONE = 0,
    TRIGGER_BUTTON,
    TRIGGER_TIMER,
    TRIGGER_MODE_SWITCH,
    TRIGGER_SHUTDOWN,
} trigger_t;

typedef enum {
    POWER_STATE_ACTIVE = 0,
    POWER_STATE_LIGHT_SLEEP,
    POWER_STATE_DEEP_SLEEP,
} power_state_t;

typedef enum {
    PERIPH_CAMERA = 0,
    PERIPH_AUDIO_AMP,
    PERIPH_VIBRATION,
} peripheral_t;

void power_manager_init(void);
trigger_t power_manager_wait_for_trigger(void);
void power_manager_light_sleep(void);
void power_manager_deep_sleep(void);
void power_manager_set_mode(int mode);
int power_manager_get_mode(void);
void power_manager_periph_enable(peripheral_t periph, bool enable);
uint32_t power_manager_read_battery_mv(void);
bool power_manager_is_battery_low(void);
power_state_t power_manager_get_state(void);
