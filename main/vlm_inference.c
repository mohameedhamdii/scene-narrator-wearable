#include "vlm_inference.h"
#include "model_loader.h"
#include "config.h"
#include "tflite_runner.h"

#include "esp_log.h"

#include <string.h>
#include <stdio.h>

static const char *TAG = "vlm";

static bool s_initialized = false;
static tflite_runner_t *s_runner=NULL;

static const char *FALLBACK_CAPTIONS[] ={
    "A sidewalk with a few pedestrians and a parked car on the left.",
    "An indoor hallway with doors on both sides and bright overhead lighting.",
    "A street corner with a crosswalk and traffic light ahead.",
    "A room with a table, chairs, and a window letting in daylight.",
};
static const size_t FALLBACK_COUNT = sizeof(FALLBACK_CAPTIONS)/ sizeof(FALLBACK_CAPTIONS[0]);

static uint32_t simple_hash(const uint8_t *data, size_t len)
{
    uint32_t h = 2166136261u;
    for(size_t i=0;i < len;i++) {


        h ^= data[i];
        h *= 16777619u;
    }
    return h;

}

static bool is_tflite(const uint8_t *data,size_t len)
{
    return data && len >= 4 && data[0]== 'T' && data[1] == 'F' && data[2] == 'L' && data[3]== '3';
}

bool vlm_inference_init(void)
{


    if (!model_loader_load(MODEL_VLM)) {
        ESP_LOGW(TAG, "VLM model not loaded: using fallback captions");
    } else{
        const uint8_t *weights = model_loader_get_data(MODEL_VLM);
        size_t size =model_loader_get_size(MODEL_VLM);
        if (is_tflite(weights, size)){


            s_runner = tflite_runner_create(weights, size);
            if (s_runner) {
                ESP_LOGI(TAG, "TFLite VLM ready");
            }
        } else {
            ESP_LOGW(TAG, "VLM weights are not TFLite: place mobilenet_v2_quant.tflite as vlm_model.tflite");
        }
    }

    s_initialized = true;
    ESP_LOGI(TAG, "VLM inference ready");
    return true;
}

bool vlm_inference_describe(const image_buffer_t *img, char *caption,size_t caption_len)
{
    if (!s_initialized || !img || !caption || caption_len == 0){
        return false;
    }

    if(s_runner && tflite_runner_describe_scene(s_runner,img->data,img->len,
                                                 caption, caption_len)) {
        ESP_LOGI(TAG,"Caption: %s",caption);
        return true;
    }

    uint32_t h = simple_hash(img->data, img->len);
    const char *desc = FALLBACK_CAPTIONS[h % FALLBACK_COUNT];
    snprintf(caption,caption_len,"%s",desc);

    ESP_LOGI(TAG, "Caption (fallback): %s", caption);
    return true;
}

void vlm_inference_deinit(void)
{
    if(s_runner) {


        tflite_runner_destroy(s_runner);
        s_runner = NULL;
    }
    s_initialized =false;
}
