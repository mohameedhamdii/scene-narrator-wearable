#pragma once

#include "camera_capture.h"

#include <stddef.h>
#include <stdbool.h>

bool ocr_engine_init(void);
bool ocr_engine_read(const image_buffer_t *img, char *text, size_t text_len);
void ocr_engine_deinit(void);
