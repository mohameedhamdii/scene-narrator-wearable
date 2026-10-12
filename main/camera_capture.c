#include "camera_capture.h"
#include "config.h"
#include "power_manager.h"

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdlib.h>
#include <string.h>

#if !CONFIG_SCENE_NARRATOR_SIMULATE
#include "esp_camera.h"
#endif

static const char *TAG = "camera";

static bool s_camera_driver_loaded =false;

#if CONFIG_SCENE_NARRATOR_SIMULATE
// minimal 1x1 jpeg for simulator tests
static const uint8_t SIM_JPEG[]= {
    0xFF, 0xD8, 0xFF, 0xE0, 0x00,0x10, 0x4A,0x46, 0x49, 0x46,0x00,0x01,
    0x01,0x00, 0x00,0x01,0x00, 0x01,0x00, 0x00, 0xFF,0xDB,0x00,0x43,
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,
    0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF, 0xFF, 0xFF,0xFF,0xFF, 0xFF,0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF,0xFF, 0xFF,0xFF, 0xFF, 0xC0, 0x00,0x0B,0x08,0x00,0x01,0x00,
    0x01, 0x01, 0x01,0x11, 0x00,0xFF,0xC4, 0x00, 0x14, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,0x00,0xFF, 0xDA,0x00, 0x08,0x01, 0x01,0x00,0x00, 0x3F,
    0x00, 0x7F, 0xFF, 0xD9
};
#endif

static image_buffer_t *alloc_image_buffer(size_t len, int quality_fallback)
{
    (void)quality_fallback;
    image_buffer_t *img = calloc(1, sizeof(image_buffer_t));
    if (!img) {
        return NULL;


    }

    img->data = heap_caps_malloc(len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if(!img->data) {
        free(img);

        return NULL;
    }

    img->owns_data = true;
    img->width = CAMERA_FRAME_WIDTH;
    img->height = CAMERA_FRAME_HEIGHT;


    return img;
}

#if !CONFIG_SCENE_NARRATOR_SIMULATE
static esp_err_t init_camera_driver(int jpeg_quality)
{
    camera_config_t cfg={
        .pin_pwdn= PIN_CAMERA_PWR,
        .pin_reset=-1,
        .pin_xclk= 18,
        .pin_sccb_sda = 21,
        .pin_sccb_scl = 22,
        .pin_d7 = 15,
        .pin_d6 = 14,
        .pin_d5= 13,
        .pin_d4 = 12,
        .pin_d3 =5,
        .pin_d2= 4,
        .pin_d1 = -1,
        .pin_d0= -1,
        .pin_vsync = 23,
        .pin_href= 25,
        .pin_pclk =19,
        .xclk_freq_hz= 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format =PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_VGA,
        .jpeg_quality =jpeg_quality,
        .fb_count=1,
        .fb_location= CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    esp_err_t err = esp_camera_init(&cfg);


    if (err == ESP_OK) {
        s_camera_driver_loaded=true;
    }
    return err;
}

static void deinit_camera_driver(void)
{
    if(s_camera_driver_loaded){
        esp_camera_deinit();

        s_camera_driver_loaded=false;
    }
}
#endif

bool camera_is_available(void)
{
#if CONFIG_SCENE_NARRATOR_SIMULATE
    return true;
#else
    return s_camera_driver_loaded || init_camera_driver(CAMERA_JPEG_QUALITY) == ESP_OK;
#endif
}

image_buffer_t *camera_capture(void)
{
    power_manager_periph_enable(PERIPH_CAMERA,true);
    vTaskDelay(pdMS_TO_TICKS(CAMERA_STARTUP_MS));

#if CONFIG_SCENE_NARRATOR_SIMULATE
    image_buffer_t *img= alloc_image_buffer(sizeof(SIM_JPEG), 0);
    if (!img) {
        power_manager_periph_enable(PERIPH_CAMERA,false);
        return NULL;
    }
    memcpy(img->data, SIM_JPEG, sizeof(SIM_JPEG));
    img->len= sizeof(SIM_JPEG);
    power_manager_periph_enable(PERIPH_CAMERA, false);
    ESP_LOGI(TAG, "Simulated capture: %u bytes",(unsigned)img->len);
    return img;


#else
    esp_err_t err= init_camera_driver(CAMERA_JPEG_QUALITY);
    if (err != ESP_OK){
        ESP_LOGW(TAG, "Camera init failed, retrying...");
        vTaskDelay(pdMS_TO_TICKS(50));
        err= init_camera_driver(CAMERA_JPEG_QUALITY);
    }
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Camera init failed: %s",esp_err_to_name(err));
        power_manager_periph_enable(PERIPH_CAMERA,false);
        return NULL;
    }

    camera_fb_t *fb=esp_camera_fb_get();
    if (!fb) {
        deinit_camera_driver();


        power_manager_periph_enable(PERIPH_CAMERA, false);
        return NULL;
    }

    image_buffer_t *img= alloc_image_buffer(fb->len, CAMERA_JPEG_QUALITY_FALLBACK);
    if (!img) {
        esp_camera_fb_return(fb);
        deinit_camera_driver();
        power_manager_periph_enable(PERIPH_CAMERA, false);
        return NULL;
    }

    memcpy(img->data, fb->buf, fb->len);
    img->len = fb->len;
    img->width = fb->width;
    img->height = fb->height;

    esp_camera_fb_return(fb);
    deinit_camera_driver();
    power_manager_periph_enable(PERIPH_CAMERA, false);

    ESP_LOGI(TAG,"Captured %ux%u JPEG (%u bytes)",img->width, img->height, (unsigned)img->len);
    return img;

#endif
}

void camera_free(image_buffer_t *img)
{
    if(!img){
        return;
    }
    if (img->owns_data && img->data) {
        heap_caps_free(img->data);
    }

    free(img);
}
