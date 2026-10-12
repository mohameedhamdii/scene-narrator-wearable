#include "power_manager.h"
#include "config.h"
#include "ui.h"

#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG= "power_mgr";

static int s_mode= MODE_ON_DEMAND;
static power_state_t s_state = POWER_STATE_ACTIVE;
static int64_t s_last_activity_us =0;
static adc_oneshot_unit_handle_t s_adc_handle;
static bool s_battery_adc_ready = false;

static void configure_wakeup_sources(void)
{
    esp_sleep_enable_ext0_wakeup(PIN_BUTTON, 0);
    if(s_mode == MODE_CONTINUOUS) {


        esp_sleep_enable_timer_wakeup((uint64_t)CONTINUOUS_INTERVAL_S * 1000000ULL);
    }
}

void power_manager_init(void)
{

    gpio_config_t pwr_cfg = {
        .pin_bit_mask = (1ULL << PIN_CAMERA_PWR) | (1ULL << PIN_AMP_SD) | (1ULL << PIN_VIBRATION),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en=GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&pwr_cfg);

    power_manager_periph_enable(PERIPH_CAMERA,false);
    power_manager_periph_enable(PERIPH_AUDIO_AMP,false);


    power_manager_periph_enable(PERIPH_VIBRATION, false);

#if !CONFIG_SCENE_NARRATOR_SIMULATE && CONFIG_SCENE_NARRATOR_BATTERY_MONITOR
    adc_oneshot_unit_init_cfg_t adc_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode =ADC_ULP_MODE_DISABLE,
    };
    if(adc_oneshot_new_unit(&adc_cfg,&s_adc_handle)== ESP_OK){
        adc_oneshot_chan_cfg_t chan_cfg = {
            .bitwidth= ADC_BITWIDTH_DEFAULT,
            .atten =ADC_ATTEN_DB_12,
        };


        if(adc_oneshot_config_channel(s_adc_handle, ADC_CHANNEL_7, &chan_cfg) == ESP_OK) {
            s_battery_adc_ready = true;
            ESP_LOGI(TAG, "Battery ADC enabled on GPIO %d",PIN_BATTERY_ADC);
        }
    }
#else
    (void)s_adc_handle;
#endif

    configure_wakeup_sources();
    s_last_activity_us =esp_timer_get_time();

    ESP_LOGI(TAG, "Power manager initialized (mode=%s)",
             s_mode == MODE_ON_DEMAND ? "on-demand" : "continuous");
}

void power_manager_periph_enable(peripheral_t periph, bool enable)
{
    switch (periph){
    case PERIPH_CAMERA:
        gpio_set_level(PIN_CAMERA_PWR, enable ? 1 : 0);
        break;
    case PERIPH_AUDIO_AMP:
        gpio_set_level(PIN_AMP_SD,enable ? 1 : 0);
        break;
    case PERIPH_VIBRATION:
        gpio_set_level(PIN_VIBRATION,enable ? 1 : 0);
        break;
    }
}

uint32_t power_manager_read_battery_mv(void)
{
#if CONFIG_SCENE_NARRATOR_SIMULATE || !CONFIG_SCENE_NARRATOR_BATTERY_MONITOR
    return 3700;
#else
    if (!s_battery_adc_ready){
        return 3700;
    }
    int raw = 0;
    if (adc_oneshot_read(s_adc_handle, ADC_CHANNEL_7, &raw) != ESP_OK) {
        return 3700;


    }
    return(uint32_t)((raw * 3300 / 4095) * 2);
#endif
}

bool power_manager_is_battery_low(void)
{
    return s_battery_adc_ready && power_manager_read_battery_mv()< BATTERY_LOW_MV;
}

void power_manager_set_mode(int mode)
{
    s_mode = mode;
    configure_wakeup_sources();
    ESP_LOGI(TAG,"Mode set to %s",mode == MODE_ON_DEMAND ? "on-demand" : "continuous");
}

int power_manager_get_mode(void)
{
    return s_mode;
}



power_state_t power_manager_get_state(void)
{
    return s_state;
}

trigger_t power_manager_wait_for_trigger(void)
{
    if(power_manager_is_battery_low()){
        ESP_LOGW(TAG,"Battery low (%lu mV)",(unsigned long)power_manager_read_battery_mv());
        return TRIGGER_SHUTDOWN;
    }

    ui_event_t event = ui_wait_for_event(
        s_mode == MODE_CONTINUOUS ? pdMS_TO_TICKS(CONTINUOUS_INTERVAL_S * 1000) : portMAX_DELAY);
    s_last_activity_us=esp_timer_get_time();

    switch(event){
    case UI_EVENT_SHORT_PRESS:
        return TRIGGER_BUTTON;
    case UI_EVENT_DOUBLE_PRESS:
        return TRIGGER_MODE_SWITCH;
    case UI_EVENT_LONG_PRESS:
        return TRIGGER_SHUTDOWN;
    case UI_EVENT_NONE:
        return s_mode == MODE_CONTINUOUS ? TRIGGER_TIMER : TRIGGER_NONE;
    default:
        return TRIGGER_NONE;
    }
}

void power_manager_light_sleep(void)
{

    s_state =POWER_STATE_LIGHT_SLEEP;

    int64_t idle_s =(esp_timer_get_time()- s_last_activity_us)/ 1000000LL;
    if (idle_s >= DEEP_SLEEP_TIMEOUT_S) {
        ESP_LOGI(TAG,"Idle timeout: entering deep sleep");
        power_manager_deep_sleep();
        return;
    }

    power_manager_periph_enable(PERIPH_CAMERA,false);

    power_manager_periph_enable(PERIPH_AUDIO_AMP, false);

#if CONFIG_SCENE_NARRATOR_SIMULATE
    vTaskDelay(pdMS_TO_TICKS(100));
#else
    configure_wakeup_sources();
    esp_light_sleep_start();
#endif
    s_state= POWER_STATE_ACTIVE;
}

void power_manager_deep_sleep(void)
{
    s_state=POWER_STATE_DEEP_SLEEP;
    power_manager_periph_enable(PERIPH_CAMERA, false);
    power_manager_periph_enable(PERIPH_AUDIO_AMP, false);
    power_manager_periph_enable(PERIPH_VIBRATION, false);



    configure_wakeup_sources();
    ESP_LOGI(TAG,"Entering deep sleep");
    esp_deep_sleep_start();
}
