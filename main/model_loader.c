#include "model_loader.h"
#include "config.h"

#include "esp_log.h"
#include "esp_heap_caps.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "model_loader";

typedef struct {
    uint8_t *data;
    size_t size;
    bool loaded;
}model_slot_t;

static model_slot_t s_models[MODEL_COUNT];

static const char *model_paths[MODEL_COUNT]= {
    [MODEL_VLM] = VLM_MODEL_PATH,
    [MODEL_OCR_DETECTOR] ="/spiffs/text_detector.tflite",
    [MODEL_OCR_RECOGNISER] = "/spiffs/text_recogniser.tflite",
    [MODEL_TTS] = TTS_MODEL_PATH,
};



#if CONFIG_SCENE_NARRATOR_SIMULATE
static bool load_simulated_model(model_type_t type)
{
    static const char SIM_VLM[]= "SIMULATED_VLM_WEIGHTS";
    static const char SIM_TFLITE[]= "TFL3SIM";

    size_t sim_size = (type == MODEL_VLM) ? sizeof(SIM_VLM) : sizeof(SIM_TFLITE);

    const char *sim_data = (type == MODEL_VLM) ? SIM_VLM : SIM_TFLITE;

    s_models[type].data = heap_caps_malloc(sim_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_models[type].data) {
        return false;
    }


    memcpy(s_models[type].data, sim_data,sim_size);
    s_models[type].size = sim_size;
    s_models[type].loaded = true;
    return true;
}
#endif

static bool load_from_filesystem(model_type_t type)
{


    const char *path = model_paths[type];
    FILE *f= fopen(path,"rb");
    if(!f){
        ESP_LOGW(TAG,"Cannot open %s", path);
        return false;
    }

    fseek(f, 0,SEEK_END);
    long file_size = ftell(f);

    fseek(f, 0, SEEK_SET);

    if(file_size <= 0){
        fclose(f);
        return false;
    }

    s_models[type].data = heap_caps_malloc((size_t)file_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if(!s_models[type].data){
        fclose(f);
        return false;
    }

    size_t read = fread(s_models[type].data, 1, (size_t)file_size, f);
    fclose(f);

    if(read != (size_t)file_size) {
        heap_caps_free(s_models[type].data);
        s_models[type].data=NULL;
        return false;
    }

    s_models[type].size = (size_t)file_size;
    s_models[type].loaded = true;
    ESP_LOGI(TAG, "Loaded %s (%u bytes)", path, (unsigned)s_models[type].size);


    return true;
}

bool model_loader_init(const char *vlm_path)
{
    if (vlm_path) {
        model_paths[MODEL_VLM] =vlm_path;
    }

    for (int i= 0; i < MODEL_COUNT;i++){
        s_models[i].data = NULL;
        s_models[i].size= 0;
        s_models[i].loaded=false;
    }

    bool vlm_ok = model_loader_load(MODEL_VLM);
    ESP_LOGI(TAG, "Model loader init: VLM %s",vlm_ok ? "ready" : "fallback mode");
    return true;


}

bool model_loader_load(model_type_t type)
{
    if (type >= MODEL_COUNT || s_models[type].loaded) {
        return s_models[type].loaded;
    }

#if CONFIG_SCENE_NARRATOR_SIMULATE
    return load_simulated_model(type);
#else
    if(load_from_filesystem(type)){
        return true;
    }
    ESP_LOGW(TAG, "Model type %d not found on filesystem", type);
    return false;
#endif
}

const uint8_t *model_loader_get_data(model_type_t type)
{
    if (type >= MODEL_COUNT || !s_models[type].loaded) {
        return NULL;
    }
    return s_models[type].data;
}

size_t model_loader_get_size(model_type_t type)
{

    if (type >= MODEL_COUNT || !s_models[type].loaded) {
        return 0;
    }
    return s_models[type].size;
}

void model_loader_deinit(void)
{
    for(int i=0;i < MODEL_COUNT;i++){
        if(s_models[i].data){

            heap_caps_free(s_models[i].data);
            s_models[i].data =NULL;
        }
        s_models[i].size = 0;
        s_models[i].loaded =false;
    }
}
