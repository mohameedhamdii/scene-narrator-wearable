#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool audio_player_init(void);
bool audio_player_play(const int16_t *samples, size_t sample_count, float gain);
void audio_player_stop(void);
void audio_player_deinit(void);
