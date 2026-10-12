#include "ui.h"
#include "config.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "led_strip.h"

static const char *TAG = "ui";

static led_state_t s_led_state = LED_OFF;
static volatile ui_event_t s_pending_event=UI_EVENT_NONE;

static int64_t s_last_release_us=0;
static int s_press_count=0;
static TimerHandle_t s_short_press_timer=NULL;

#if !CONFIG_SCENE_NARRATOR_SIMULATE
static led_strip_handle_t s_led_strip = NULL;
static TaskHandle_t s_led_task= NULL;
static volatile bool s_led_animating= false;



static void led_strip_set_rgb(uint8_t r,uint8_t g,uint8_t b)
{
    if (!s_led_strip) {
        return;
    }

    led_strip_set_pixel(s_led_strip,0,r,g,b);
    led_strip_refresh(s_led_strip);
}

static void led_animation_task(void *arg)
{
    (void)arg;


    while (1){
        if (!s_led_animating) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        if(s_led_state == LED_BLUE_PULSE){


            for (int i= 0;i <= 255 && s_led_animating;i += 8){
                led_strip_set_rgb(0, 0, (uint8_t)i);
                vTaskDelay(pdMS_TO_TICKS(15));
            }
            for (int i = 255; i >= 0 && s_led_animating; i -= 8) {
                led_strip_set_rgb(0, 0, (uint8_t)i);
                vTaskDelay(pdMS_TO_TICKS(15));
            }

        } else if (s_led_state == LED_RED_BLINK) {
            for (int n = 0;n < 6 && s_led_animating;n++) {
                led_strip_set_rgb(255, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(200));
                led_strip_set_rgb(0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            s_led_animating=false;
        } else{
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

static void led_hw_init(void)
{
    led_strip_config_t strip_cfg = {
        .strip_gpio_num =PIN_LED_DATA,
        .max_leds=1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags={ .invert_out =false },
    };
    led_strip_rmt_config_t rmt_cfg = {
        .clk_src=RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags = { .with_dma = false },
    };
    if(led_strip_new_rmt_device(&strip_cfg,&rmt_cfg, &s_led_strip)!= ESP_OK) {
        ESP_LOGW(TAG, "WS2812 init failed: LED disabled");


        return;
    }
    xTaskCreate(led_animation_task,"led_anim", 2048,NULL,3, &s_led_task);
}
#endif

static void led_apply_solid(uint8_t r, uint8_t g, uint8_t b)
{
#if CONFIG_SCENE_NARRATOR_SIMULATE
    const char *names[] = { "OFF","GREEN", "BLUE_PULSE", "WHITE", "RED_BLINK"};
    ESP_LOGD(TAG,"LED -> %s",names[s_led_state]);
    (void)r;
    (void)g;
    (void)b;
#else
    if (!s_led_strip){
        return;
    }
    s_led_animating = false;


    vTaskDelay(pdMS_TO_TICKS(10));
    if(r == 0 && g == 0 && b == 0){
        led_strip_clear(s_led_strip);
        led_strip_refresh(s_led_strip);
    } else {
        led_strip_set_rgb(r, g, b);
    }
#endif
}

static void led_task_apply(led_state_t state)
{
    s_led_state =state;
#if !CONFIG_SCENE_NARRATOR_SIMULATE
    if (!s_led_strip) {
        return;
    }
#endif

    switch(state){
    case LED_OFF:
        led_apply_solid(0, 0, 0);
        break;
    case LED_GREEN_SOLID:
        led_apply_solid(0, 180, 0);
        break;
    case LED_BLUE_PULSE:
#if !CONFIG_SCENE_NARRATOR_SIMULATE
        s_led_animating =true;

#else
        led_apply_solid(0, 0,180);
#endif
        break;
    case LED_WHITE_SOLID:
        led_apply_solid(200, 200, 200);
        break;
    case LED_RED_BLINK:
#if !CONFIG_SCENE_NARRATOR_SIMULATE
        s_led_animating=true;
#else
        led_apply_solid(255, 0, 0);
#endif
        break;
    }

}

static void haptic_buzz(int duration_ms)
{
    gpio_set_level(PIN_VIBRATION, 1);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    gpio_set_level(PIN_VIBRATION, 0);
}

static void short_press_timer_cb(TimerHandle_t timer)
{
    (void)timer;
    if (s_press_count == 1 && s_pending_event == UI_EVENT_NONE) {
        s_pending_event = UI_EVENT_SHORT_PRESS;
        s_press_count = 0;


    }
}

static void button_task(void *arg)
{
    (void)arg;
    bool was_down= false;
    int64_t down_us = 0;

    s_short_press_timer=xTimerCreate("btn_defer",pdMS_TO_TICKS(UI_DOUBLE_PRESS_MS),
                                        pdFALSE,NULL, short_press_timer_cb);

    while (1) {
#if CONFIG_SCENE_NARRATOR_SIMULATE
        vTaskDelay(pdMS_TO_TICKS(2000));
        s_pending_event=UI_EVENT_SHORT_PRESS;


        continue;

#else
        int level=gpio_get_level(PIN_BUTTON);
        int64_t now=esp_timer_get_time();

        if(level == 0 && !was_down){
            was_down = true;
            down_us=now;


            xTimerStop(s_short_press_timer, 0);
        } else if(level != 0 && was_down){
            was_down = false;
            int64_t duration_ms= (now - down_us)/ 1000;

            if (duration_ms >= UI_LONG_PRESS_MS) {
                xTimerStop(s_short_press_timer, 0);
                s_press_count = 0;
                s_pending_event = UI_EVENT_LONG_PRESS;

            } else if (duration_ms <= UI_SHORT_PRESS_MS) {
                if (s_last_release_us > 0 &&
                    (now - s_last_release_us) <(int64_t)UI_DOUBLE_PRESS_MS * 1000LL){
                    s_press_count++;
                } else{
                    s_press_count = 1;
                }
                s_last_release_us=now;

                if (s_press_count >= 2) {
                    xTimerStop(s_short_press_timer, 0);


                    s_press_count = 0;
                    s_pending_event= UI_EVENT_DOUBLE_PRESS;
                } else {
                    xTimerStop(s_short_press_timer, 0);
                    xTimerStart(s_short_press_timer, 0);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(UI_DEBOUNCE_MS));
#endif
    }
}

void ui_init(void)
{
#if !CONFIG_SCENE_NARRATOR_SIMULATE
    gpio_config_t btn_cfg= {
        .pin_bit_mask = 1ULL << PIN_BUTTON,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type =GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_cfg);
    led_hw_init();
#endif

    xTaskCreate(button_task,"ui_btn", 3072, NULL,5,NULL);

    ESP_LOGI(TAG,"UI initialized");
    ui_set_led(LED_GREEN_SOLID);
}

ui_event_t ui_wait_for_event(TickType_t timeout)
{
    TickType_t start = xTaskGetTickCount();
    TickType_t wait_ticks = (timeout == portMAX_DELAY) ? pdMS_TO_TICKS(60000) : timeout;

    while (1) {
        if(s_pending_event != UI_EVENT_NONE) {
            ui_event_t ev=s_pending_event;
            s_pending_event = UI_EVENT_NONE;

            return ev;

        }
        if ((xTaskGetTickCount()- start)>= wait_ticks) {
            return UI_EVENT_NONE;
        }
        vTaskDelay(pdMS_TO_TICKS(UI_DEBOUNCE_MS));
    }
}

void ui_set_led(led_state_t state)
{
    led_task_apply(state);
}

void ui_haptic(haptic_pattern_t pattern)
{
    switch (pattern) {
    case HAPTIC_SHORT:
        haptic_buzz(UI_HAPTIC_SHORT_MS);
        break;
    case HAPTIC_DOUBLE:
        haptic_buzz(UI_HAPTIC_SHORT_MS);
        vTaskDelay(pdMS_TO_TICKS(80));
        haptic_buzz(UI_HAPTIC_SHORT_MS);
        break;
    case HAPTIC_LONG:
        haptic_buzz(UI_HAPTIC_LONG_MS);
        break;
    }


}
