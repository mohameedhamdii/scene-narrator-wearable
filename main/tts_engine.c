#include "tts_engine.h"
#include "audio_player.h"
#include "model_loader.h"
#include "config.h"
#include "piper_lite.h"

#include "esp_log.h"

#include <stdlib.h>
#include <string.h>

static const char *TAG = "tts";

static bool s_initialized = false;
static float s_gain =AUDIO_DEFAULT_GAIN;

bool tts_engine_init(void)
{
    const uint8_t *model= NULL;
    size_t model_size =0;

    if (model_loader_load(MODEL_TTS)) {
        model = model_loader_get_data(MODEL_TTS);
        model_size =model_loader_get_size(MODEL_TTS);


    } else {
        ESP_LOGW(TAG, "Piper model not found: using phoneme synthesizer");
    }

    piper_lite_init(model, model_size);

    s_initialized=true;
    ESP_LOGI(TAG,"TTS engine initialized");
    return true;
}

bool tts_speak(const char *text)
{


    if (!s_initialized || !text || text[0] == '\0') {
        return false;
    }

    int16_t *samples =NULL;
    size_t sample_count = 0;

    if (!piper_lite_synthesize(text, &samples, &sample_count)) {


        return false;
    }

    bool ok = audio_player_play(samples, sample_count, s_gain);
    free(samples);

    ESP_LOGI(TAG, "Spoke: \"%.40s%s\"", text, strlen(text) > 40 ? "..." : "");
    return ok;
}

void tts_engine_deinit(void)
{

    piper_lite_deinit();
    s_initialized = false;
}
