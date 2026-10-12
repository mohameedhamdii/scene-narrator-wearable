#include "piper_lite.h"
#include "phoneme_synth.h"

#include "esp_log.h"

#include <stdlib.h>
#include <string.h>

static const char *TAG= "piper";

static const uint8_t *s_model_data;
static size_t s_model_size;

bool piper_lite_init(const uint8_t *model_data,size_t model_size)
{
    s_model_data= model_data;
    s_model_size = model_size;

    if (model_data && model_size > 4){
        bool is_onnx = model_data[0] == 0x08 ||(model_data[0] == 'O' && model_data[1] == 'N');
        ESP_LOGI(TAG, "TTS model loaded (%u bytes, onnx=%d)", (unsigned)model_size, is_onnx);


    } else{
        ESP_LOGW(TAG,"No Piper model: using phoneme synthesizer");
    }
    return true;

}

void piper_lite_deinit(void)
{
    s_model_data = NULL;
    s_model_size = 0;
}



bool piper_lite_has_model(void)
{
    return s_model_data != NULL && s_model_size > 0;
}

bool piper_lite_synthesize(const char *text, int16_t **samples, size_t *sample_count)
{
    if(!text || !samples || !sample_count) {
        return false;


    }

    // fallback phoneme formant synthesis when onnx runtime is not linked
    (void)s_model_data;
    (void)s_model_size;

    if(!phoneme_synth_text(text, 16000, samples,sample_count)) {
        ESP_LOGE(TAG, "Phoneme synthesis failed");
        return false;
    }

    ESP_LOGI(TAG,"Synthesized %u samples for \"%.32s%s\"",
             (unsigned)*sample_count,text,strlen(text)> 32 ? "..." : "");

    return true;
}
