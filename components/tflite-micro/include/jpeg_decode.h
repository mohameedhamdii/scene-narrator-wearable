#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *rgb;
    int width;
    int height;
} rgb_image_t;

bool jpeg_decode_to_rgb(const uint8_t *jpeg, size_t jpeg_len, rgb_image_t *out);
void jpeg_decode_free(rgb_image_t *img);

bool jpeg_decode_to_rgb_resized(const uint8_t *jpeg, size_t jpeg_len,
                                int target_w, int target_h, rgb_image_t *out);
