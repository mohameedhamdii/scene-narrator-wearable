#include "ocr_engine.h"
#include "model_loader.h"
#include "config.h"
#include "tflite_runner.h"

#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "ocr";

static bool s_initialized = false;
static tflite_runner_t *s_detector =NULL;
static tflite_runner_t *s_recogniser = NULL;

static bool is_tflite(const uint8_t *data,size_t len)
{
    return data && len >= 4 && data[0] == 'T' && data[1]== 'F' && data[2]== 'L' && data[3]== '3';
}

bool ocr_engine_init(void)
{
    model_loader_load(MODEL_OCR_DETECTOR);


    model_loader_load(MODEL_OCR_RECOGNISER);

    const uint8_t *det = model_loader_get_data(MODEL_OCR_DETECTOR);
    size_t det_size=model_loader_get_size(MODEL_OCR_DETECTOR);
    if (is_tflite(det,det_size)){

        s_detector = tflite_runner_create(det,det_size);
    }

    const uint8_t *rec = model_loader_get_data(MODEL_OCR_RECOGNISER);
    size_t rec_size = model_loader_get_size(MODEL_OCR_RECOGNISER);
    if(is_tflite(rec, rec_size)){


        s_recogniser = tflite_runner_create(rec, rec_size);
    }

    s_initialized = true;
    ESP_LOGI(TAG,"OCR engine initialized (det=%d rec=%d)", s_detector != NULL, s_recogniser != NULL);
    return true;
}



bool ocr_engine_read(const image_buffer_t *img, char *text, size_t text_len)
{
    if (!s_initialized || !img || !text || text_len == 0) {
        return false;
    }

    text[0] = '\0';

    if (!s_detector || !s_recogniser) {
        ESP_LOGD(TAG,"OCR models not loaded, skipping detection");
        return true;

    }

    tflite_text_box_t boxes[OCR_MAX_REGIONS];
    int count = 0;
    if (!tflite_runner_detect_text(s_detector, img->data, img->len,
                                   boxes, OCR_MAX_REGIONS,&count)|| count == 0){
        ESP_LOGD(TAG,"No text detected");
        return true;
    }

    char fragment[64];
    for (int i =0;i < count && i < OCR_MAX_REGIONS;i++){
        if(!tflite_runner_recognise_text(s_recogniser,img->data, img->len,
                                          &boxes[i], fragment,sizeof(fragment))){
            continue;
        }
        if (text[0] != '\0') {
            strncat(text, "; ", text_len - strlen(text) - 1);
        }
        strncat(text, fragment, text_len - strlen(text) - 1);
    }

    ESP_LOGI(TAG, "OCR result: %s", text[0] ? text : "(none)");
    return true;
}



void ocr_engine_deinit(void)
{
    if (s_detector) {
        tflite_runner_destroy(s_detector);
        s_detector = NULL;
    }
    if(s_recogniser) {
        tflite_runner_destroy(s_recogniser);
        s_recogniser= NULL;
    }
    s_initialized = false;
}
