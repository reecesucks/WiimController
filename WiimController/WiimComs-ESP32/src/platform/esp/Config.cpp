#include "Config.h"

#include <vector>

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_err.h"

#include "Log.h"

#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #include "secrets.example.h"
#endif

namespace {
constexpr const char* TAG = "wiim.config";
constexpr const char* NVS_NS = "wiimcfg";
constexpr const char* KEY_SSID     = "ssid";
constexpr const char* KEY_PASSWORD = "password";
constexpr const char* KEY_BASE_URL = "base_url";

bool readString(nvs_handle_t h, const char* key, std::string& out) {
    size_t len = 0;
    esp_err_t err = nvs_get_str(h, key, nullptr, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return false;  // first boot, key never written
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_get_str length probe failed for '%s': %s",
                 key, esp_err_to_name(err));
        return false;
    }
    if (len == 0) {
        out.clear();
        return true;
    }

    std::vector<char> buf(len);
    err = nvs_get_str(h, key, buf.data(), &len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_get_str read failed for '%s': %s",
                 key, esp_err_to_name(err));
        return false;
    }

    out.assign(buf.data());
    return true;
}

esp_err_t writeString(nvs_handle_t h, const char* key, const std::string& value) {
    esp_err_t err = nvs_set_str(h, key, value.c_str());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_set_str failed for '%s': %s",
                 key, esp_err_to_name(err));
    }
    return err;
}
}


AppConfig loadConfig() {
    AppConfig cfg;

    cfg.wifiSsid     = WIFI_SSID;
    cfg.wifiPassword = WIFI_PASSWORD;
    cfg.wiimBaseUrl  = WIIM_BASE_URL;

    nvs_handle_t h = 0;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "no NVS namespace yet, using compile-time defaults");
        return cfg;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open(readonly) failed: %s -- using defaults",
                 esp_err_to_name(err));
        return cfg;
    }

    int loaded = 0;
    if (readString(h, KEY_SSID,     cfg.wifiSsid))     loaded++;
    if (readString(h, KEY_PASSWORD, cfg.wifiPassword)) loaded++;
    if (readString(h, KEY_BASE_URL, cfg.wiimBaseUrl))  loaded++;

    nvs_close(h);

    ESP_LOGI(TAG, "config loaded (%d/3 fields from NVS, rest from defaults)", loaded);
    return cfg;
}

void saveConfig(const AppConfig& cfg) {
    nvs_handle_t h = 0;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open(rw) failed: %s -- cannot save",
                 esp_err_to_name(err));
        return;
    }

    writeString(h, KEY_SSID,     cfg.wifiSsid);
    writeString(h, KEY_PASSWORD, cfg.wifiPassword);
    writeString(h, KEY_BASE_URL, cfg.wiimBaseUrl);

    err = nvs_commit(h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_commit failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "config saved to NVS");
    }

    nvs_close(h);
}