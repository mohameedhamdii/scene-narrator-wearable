#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool piper_lite_init(const uint8_t *model_data, size_t model_size);
void piper_lite_deinit(void);

/* Synthesize text to 16 kHz mono PCM. Caller frees *samples with free(). */
bool piper_lite_synthesize(const char *text, int16_t **samples, size_t *sample_count);
bool piper_lite_has_model(void);
