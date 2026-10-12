#pragma once

#include <stdbool.h>

bool tts_engine_init(void);
bool tts_speak(const char *text);
void tts_engine_deinit(void);
