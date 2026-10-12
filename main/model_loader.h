#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MODEL_VLM = 0,
    MODEL_OCR_DETECTOR,
    MODEL_OCR_RECOGNISER,
    MODEL_TTS,
    MODEL_COUNT,
} model_type_t;

bool model_loader_init(const char *vlm_path);
bool model_loader_load(model_type_t type);
const uint8_t *model_loader_get_data(model_type_t type);
size_t model_loader_get_size(model_type_t type);
void model_loader_deinit(void);
