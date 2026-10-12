#include "spiffs_mount.h"

#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "spiffs";
static bool s_mounted = false;

bool spiffs_mount(void)
{
    if (s_mounted) {
        return true;
    }

    esp_vfs_spiffs_conf_t conf= {
        .base_path = "/spiffs",
        .partition_label= "spiffs",
        .max_files = 8,
        .format_if_mount_failed = false,
    };

    esp_err_t ret=esp_vfs_spiffs_register(&conf);


    if(ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG,"SPIFFS mount failed: flash models per models/README.md");
        } else if (ret == ESP_ERR_NOT_FOUND){

            ESP_LOGE(TAG, "SPIFFS partition not found");
        } else{
            ESP_LOGE(TAG,"SPIFFS init error: %s",esp_err_to_name(ret));
        }
        return false;


    }

    size_t total = 0;
    size_t used = 0;
    esp_spiffs_info("spiffs", &total, &used);
    ESP_LOGI(TAG, "SPIFFS mounted: %u / %u bytes used", (unsigned)used, (unsigned)total);
    s_mounted=true;


    return true;
}

void spiffs_unmount(void)
{
    if (s_mounted) {
        esp_vfs_spiffs_unregister("spiffs");
        s_mounted = false;
    }
}

