#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *data;
    size_t len;
    int width;
    int height;
    bool owns_data;
} image_buffer_t;

image_buffer_t *camera_capture(void);
void camera_free(image_buffer_t *img);
bool camera_is_available(void);
