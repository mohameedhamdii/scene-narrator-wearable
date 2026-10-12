#include "jpeg_decode.h"

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "jpeg_decoder.h"

#include <stdlib.h>
#include <string.h>

static const char *TAG = "jpeg_dec";

bool jpeg_decode_to_rgb(const uint8_t *jpeg, size_t jpeg_len, rgb_image_t *out)
{
    if(!jpeg || jpeg_len == 0 || !out) {
        return false;
    }

    uint8_t *decoded=NULL;
    int out_len=0;
    int width= 0;
    int height = 0;



    jpeg_error_t err=esp_jpeg_decode_one_picture(
        jpeg, (int)jpeg_len, &decoded,&out_len, &width, &height);
    if(err != JPEG_ERR_OK || !decoded || width <= 0 || height <= 0) {
        ESP_LOGW(TAG, "JPEG decode failed: %d", err);
        if(decoded) {

            free(decoded);
        }
        return false;
    }

    out->rgb =decoded;


    out->width = width;
    out->height = height;
    return true;
}

void jpeg_decode_free(rgb_image_t *img)
{
    if(img && img->rgb) {


        free(img->rgb);
        img->rgb = NULL;
        img->width = 0;
        img->height = 0;
    }
}

static void resize_rgb_nearest(const uint8_t *src, int sw, int sh,
                               uint8_t *dst,int dw,int dh)
{
    for (int y = 0; y < dh; y++) {

        int sy = y * sh / dh;
        for(int x= 0; x < dw;x++){
            int sx=x * sw / dw;
            const uint8_t *sp=src + (sy * sw + sx) * 3;
            uint8_t *dp=dst + (y * dw + x) * 3;
            dp[0] = sp[0];
            dp[1] = sp[1];
            dp[2] = sp[2];
        }
    }
}

bool jpeg_decode_to_rgb_resized(const uint8_t *jpeg, size_t jpeg_len,
                                int target_w, int target_h, rgb_image_t *out)
{
    rgb_image_t full= {0};
    if (!jpeg_decode_to_rgb(jpeg, jpeg_len, &full)) {
        return false;
    }

    if(full.width == target_w && full.height == target_h) {
        *out =full;
        return true;
    }



    size_t out_bytes = (size_t)target_w * (size_t)target_h * 3;
    uint8_t *resized=heap_caps_malloc(out_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!resized) {
        jpeg_decode_free(&full);
        return false;
    }

    resize_rgb_nearest(full.rgb,full.width,full.height, resized,target_w, target_h);
    jpeg_decode_free(&full);

    out->rgb =resized;
    out->width=target_w;
    out->height = target_h;
    return true;
}
