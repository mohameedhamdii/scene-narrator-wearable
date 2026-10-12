#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool phoneme_synth_text(const char *text, int sample_rate,
                        int16_t **samples, size_t *sample_count);
