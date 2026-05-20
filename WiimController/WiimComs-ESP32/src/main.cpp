#include <cstdio>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #include "secrets.example.h"
  #warning "Using secrets.example.h placeholders. Copy include/secrets.example.h to include/secrets.h."
#endif

#include "WiimService.h"
#include "platform/esp/EspHttpClient.h"

namespace {
constexpr const char* TAG = "wiim";

constexpr int WIFI_CONNECTED_BIT = BIT0;

EventGroupHandle_t s_wifi_event_group = nullptr;
EspHttpClient*     g_http             = nullptr;
WiimService*       g_wiim             = nullptr;

void wifiEventHandler(void* /*arg*/, esp_event_base_t base, int32_t id, void* data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "wifi disconnected, retrying");
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(data);
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void connectWifi() {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, nullptr, nullptr));

    wifi_config_t wifi_config = {};
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid),
                 WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password),
                 WIFI_PASSWORD, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "connecting to SSID '%s'", WIFI_SSID);
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE,
        pdMS_TO_TICKS(30000));

    if (!(bits & WIFI_CONNECTED_BIT)) {
        ESP_LOGE(TAG, "wifi connect timeout, restarting");
        esp_restart();
    }
}

void wiimTask(void* /*pv*/) {
    ESP_LOGI(TAG, "task started");

    DeviceStatus ds;
    if (g_wiim->getDeviceStatus(ds)) {
        ESP_LOGI(TAG, "device: %s (firmware %s, MAC %s)",
                 ds.deviceName.c_str(),
                 ds.firmware.c_str(),
                 ds.mac.c_str());
    } else {
        ESP_LOGW(TAG, "getDeviceStatus failed");
    }

    if (g_wiim->setPlayerStatus()) {
        ESP_LOGI(TAG, "volume=%d", g_wiim->volume());
    } else {
        ESP_LOGW(TAG, "setPlayerStatus failed");
    }

    MusicTrack track;
    if (g_wiim->getSongMetaData(track)) {
        ESP_LOGI(TAG, "now playing: %s — %s (%s)",
                 track.metaData.artist.c_str(),
                 track.metaData.title.c_str(),
                 track.metaData.album.c_str());
    } else {
        ESP_LOGW(TAG, "getSongMetaData failed");
    }

    vTaskDelete(nullptr);
}
}  // namespace

extern "C" void app_main() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "WiimComs-ESP32 starting");

    connectWifi();

    g_http = new EspHttpClient();
    g_wiim = new WiimService(g_http, WIIM_BASE_URL);

    xTaskCreatePinnedToCore(
        wiimTask,
        "wiim",
        8192,
        nullptr,
        5,
        nullptr,
        APP_CPU_NUM);
}
