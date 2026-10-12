#pragma once

#include "camera_capture.h"

#include <stddef.h>

bool vlm_inference_init(void);
bool vlm_inference_describe(const image_buffer_t *img, char *caption, size_t caption_len);
void vlm_inference_deinit(void);
