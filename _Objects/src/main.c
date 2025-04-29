#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"

#include "app_camera.h"     // ต้องมีใน include/
#include "esp_http_server.h"

static const char *TAG = "main";

void app_main(void) {
    esp_err_t ret;

    // Init NVS (สำหรับ WiFi, OTA, ฯลฯ)
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Initializing camera...");
    if (app_camera_init() != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed");
        return;
    }

    ESP_LOGI(TAG, "Starting HTTP server...");
    // ปกติ app_httpd_start() จะมีใน ESP-WHO ที่ใช้ esp_http_server
    extern esp_err_t app_httpd_start();
    app_httpd_start();

    ESP_LOGI(TAG, "System ready!");
}
