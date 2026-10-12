#include "audio_player.h"
#include "config.h"
#include "power_manager.h"

#include "esp_log.h"
#include "driver/i2s_std.h"

#include <stdlib.h>

static const char *TAG = "audio";

static i2s_chan_handle_t s_tx_handle;
static bool s_initialized = false;

bool audio_player_init(void)
{
#if CONFIG_SCENE_NARRATOR_SIMULATE
    s_initialized=true;
    ESP_LOGI(TAG,"Audio player initialized (simulated I2S)");
    return true;
#else
    i2s_chan_config_t chan_cfg= I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0,I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num= 4;


    chan_cfg.dma_frame_num = AUDIO_DMA_BUF_SAMPLES;

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &s_tx_handle,NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg= I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg= {

            .mclk =I2S_GPIO_UNUSED,
            .bclk = PIN_I2S_BCLK,
            .ws = PIN_I2S_LRCLK,
            .dout= PIN_I2S_DOUT,
            .din = I2S_GPIO_UNUSED,
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(s_tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(s_tx_handle));

    s_initialized = true;
    ESP_LOGI(TAG, "I2S audio initialized at %d Hz", AUDIO_SAMPLE_RATE);


    return true;
#endif
}

bool audio_player_play(const int16_t *samples,size_t sample_count,float gain)
{
    if (!s_initialized || !samples || sample_count == 0) {
        return false;
    }



    power_manager_periph_enable(PERIPH_AUDIO_AMP, true);

    if(gain < 0.0f){
        gain = 0.0f;
    }
    if(gain > 1.0f){
        gain=1.0f;
    }

#if CONFIG_SCENE_NARRATOR_SIMULATE
    ESP_LOGI(TAG,"Playing %u samples (gain=%.2f)",(unsigned)sample_count, gain);

    (void)gain;
#else
    int16_t *buf = malloc(sample_count * sizeof(int16_t));
    if (!buf) {
        power_manager_periph_enable(PERIPH_AUDIO_AMP, false);
        return false;
    }

    for (size_t i = 0; i < sample_count; i++) {
        int32_t scaled=(int32_t)(samples[i] * gain);
        if (scaled > 32767) {
            scaled = 32767;
        }
        if (scaled < -32768) {
            scaled = -32768;
        }
        buf[i]= (int16_t)scaled;
    }

    size_t written = 0;
    size_t bytes= sample_count * sizeof(int16_t);
    i2s_channel_write(s_tx_handle, buf,bytes,&written,portMAX_DELAY);
    free(buf);


#endif

    power_manager_periph_enable(PERIPH_AUDIO_AMP, false);
    return true;
}

void audio_player_stop(void)
{
    power_manager_periph_enable(PERIPH_AUDIO_AMP, false);
}

void audio_player_deinit(void)
{
#if !CONFIG_SCENE_NARRATOR_SIMULATE
    if(s_tx_handle){
        i2s_channel_disable(s_tx_handle);
        i2s_del_channel(s_tx_handle);
        s_tx_handle =NULL;
    }
#endif
    s_initialized = false;
}


