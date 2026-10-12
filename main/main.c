#include "config.h"
#include "power_manager.h"
#include "camera_capture.h"
#include "vlm_inference.h"
#include "ocr_engine.h"
#include "tts_engine.h"
#include "audio_player.h"
#include "ui.h"
#include "model_loader.h"
#include "spiffs_mount.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include <string.h>

static const char *TAG = "main";

static void handle_mode_switch(void)
{
    int mode = power_manager_get_mode();
    mode = (mode == MODE_ON_DEMAND)? MODE_CONTINUOUS : MODE_ON_DEMAND;
    power_manager_set_mode(mode);
    ui_haptic(HAPTIC_DOUBLE);

    const char *msg = (mode == MODE_CONTINUOUS) ? "Continuous mode" : "On demand mode";
    tts_speak(msg);
}



static bool run_narration_cycle(void)
{
    image_buffer_t *img=camera_capture();
    if(img == NULL) {
        ui_set_led(LED_RED_BLINK);

        tts_speak("Camera error");
        return false;
    }

    char caption[VLM_CAPTION_MAX_LEN];
    if (!vlm_inference_describe(img, caption, sizeof(caption))) {


        ui_set_led(LED_RED_BLINK);
        tts_speak("Inference error");
        camera_free(img);
        return false;
    }

    char ocr_text[OCR_TEXT_MAX_LEN];


    ocr_engine_read(img,ocr_text, sizeof(ocr_text));

    char full_text[384];
    if (ocr_text[0]!= '\0') {
        snprintf(full_text,sizeof(full_text),"%s. Text reads: %s",caption,ocr_text);
    } else {
        strncpy(full_text,caption, sizeof(full_text)- 1);
        full_text[sizeof(full_text)- 1] = '\0';
    }


    ui_set_led(LED_WHITE_SOLID);
    tts_speak(full_text);

    camera_free(img);
    return true;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Scene Narrator Wearable starting");

    esp_err_t ret= nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    power_manager_init();
    ui_init();
    audio_player_init();

#if !CONFIG_SCENE_NARRATOR_SIMULATE
    if (!spiffs_mount()) {
        ESP_LOGW(TAG, "SPIFFS unavailable: models must be flashed for full inference");
    }
#endif

    model_loader_init(VLM_MODEL_PATH);


    vlm_inference_init();
    ocr_engine_init();
    tts_engine_init();

    ui_set_led(LED_GREEN_SOLID);
    ESP_LOGI(TAG, "Ready: press button to narrate");

    while (1){
        trigger_t trigger = power_manager_wait_for_trigger();

        if (trigger == TRIGGER_SHUTDOWN) {
            ui_haptic(HAPTIC_LONG);
            if (power_manager_is_battery_low()) {
                tts_speak("Battery low. Shutting down.");
            }
            power_manager_deep_sleep();
        }



        if(trigger == TRIGGER_MODE_SWITCH) {
            handle_mode_switch();
            continue;
        }

        if (trigger == TRIGGER_NONE){
            continue;
        }

        ui_haptic(HAPTIC_SHORT);
        ui_set_led(LED_BLUE_PULSE);

        run_narration_cycle();
        ui_set_led(LED_GREEN_SOLID);
        power_manager_light_sleep();
    }
}
