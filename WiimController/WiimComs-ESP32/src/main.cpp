#include <cstdio>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
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
#include "platform/esp/BoardLed.h"
#include "platform/esp/Buttons.h"
#include "platform/esp/EspHttpClient.h"
#include "platform/esp/RotaryEncoder.h"

namespace {
constexpr const char* TAG = "wiim";

constexpr int WIFI_CONNECTED_BIT = BIT0;

// --- Hardware pin assignments. Change these if your wiring is different. ---
constexpr gpio_num_t PIN_LED        = GPIO_NUM_2;   // built-in blue LED on most WROOM-32 dev kits
constexpr gpio_num_t PIN_BTN_PREV   = GPIO_NUM_13;
constexpr gpio_num_t PIN_BTN_PLAY   = GPIO_NUM_14;
constexpr gpio_num_t PIN_BTN_NEXT   = GPIO_NUM_27;
constexpr gpio_num_t PIN_ROT_CLK    = GPIO_NUM_32;
constexpr gpio_num_t PIN_ROT_DT     = GPIO_NUM_33;
constexpr gpio_num_t PIN_ROT_SW     = GPIO_NUM_25;  // rotary's push switch (KEY pin)

constexpr int VOLUME_STEP = 5;   // % per detent
// --------------------------------------------------------------------------

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

int clampVolume(int v) {
    if (v < 0)   return 0;
    if (v > 100) return 100;
    return v;
}

void logApiResult(const char* what, const WiimApiResult& r) {
    if (r.success) {
        ESP_LOGI(TAG, "  -> %s OK", what);
    } else {
        ESP_LOGW(TAG, "  -> %s FAILED: %s", what, r.message.c_str());
    }
}

void wiimTask(void* /*pv*/) {
    ESP_LOGI(TAG, "=== wiim task started ===");

    ESP_LOGI(TAG, "init LED on GPIO%d", PIN_LED);
    BoardLed led(PIN_LED);
    led.begin();

    // 3 quick flashes on boot so we know the LED is alive.
    ESP_LOGI(TAG, "LED boot flash (you should see 3 quick blinks)");
    for (int i = 0; i < 3; ++i) {
        led.blink(80);
        vTaskDelay(pdMS_TO_TICKS(160));
    }

    // Initial pull of device + player state.
    ESP_LOGI(TAG, "fetching initial device status from %s", WIIM_BASE_URL);
    DeviceStatus ds;
    if (g_wiim->getDeviceStatus(ds)) {
        ESP_LOGI(TAG, "  -> device:   %s", ds.deviceName.c_str());
        ESP_LOGI(TAG, "  -> firmware: %s", ds.firmware.c_str());
        ESP_LOGI(TAG, "  -> MAC:      %s", ds.mac.c_str());
    } else {
        ESP_LOGE(TAG, "  -> getDeviceStatus FAILED -- check WIIM_BASE_URL and that the Wiim is reachable");
    }

    ESP_LOGI(TAG, "fetching initial player status");
    if (g_wiim->setPlayerStatus()) {
        ESP_LOGI(TAG, "  -> initial volume=%d", g_wiim->volume());
    } else {
        ESP_LOGW(TAG, "  -> setPlayerStatus failed (will retry on first rotary turn)");
    }

    ESP_LOGI(TAG, "wiring buttons:");
    ESP_LOGI(TAG, "  prev    -> GPIO%d", PIN_BTN_PREV);
    ESP_LOGI(TAG, "  play    -> GPIO%d", PIN_BTN_PLAY);
    ESP_LOGI(TAG, "  next    -> GPIO%d", PIN_BTN_NEXT);
    ESP_LOGI(TAG, "  rot SW  -> GPIO%d", PIN_ROT_SW);

    Buttons buttons;
    buttons.add(PIN_BTN_PREV, [&led] {
        ESP_LOGI(TAG, "btn: prev pressed");
        led.blink();
        logApiResult("playPreviousSong", g_wiim->playPreviousSong());
    });
    buttons.add(PIN_BTN_PLAY, [&led] {
        ESP_LOGI(TAG, "btn: play/pause pressed");
        led.blink();
        logApiResult("onePause", g_wiim->onePause());
    });
    buttons.add(PIN_BTN_NEXT, [&led] {
        ESP_LOGI(TAG, "btn: next pressed");
        led.blink();
        logApiResult("playNextSong", g_wiim->playNextSong());
    });
    buttons.add(PIN_ROT_SW, [&led] {
        ESP_LOGI(TAG, "rotary: knob pressed (play/pause)");
        led.blink();
        logApiResult("onePause", g_wiim->onePause());
    });
    buttons.begin();

    // Dump initial pin states so user can sanity-check wiring.
    // Released (pulled up) should read HIGH/1. If any reads LOW/0 at startup,
    // either the button is being held or it's miswired (shorted to GND).
    ESP_LOGI(TAG, "initial input states (1=released, 0=pressed):");
    ESP_LOGI(TAG, "  prev   (GPIO%d) = %d", PIN_BTN_PREV, gpio_get_level(PIN_BTN_PREV));
    ESP_LOGI(TAG, "  play   (GPIO%d) = %d", PIN_BTN_PLAY, gpio_get_level(PIN_BTN_PLAY));
    ESP_LOGI(TAG, "  next   (GPIO%d) = %d", PIN_BTN_NEXT, gpio_get_level(PIN_BTN_NEXT));
    ESP_LOGI(TAG, "  rot SW (GPIO%d) = %d", PIN_ROT_SW,   gpio_get_level(PIN_ROT_SW));

    ESP_LOGI(TAG, "wiring rotary: CLK->GPIO%d, DT->GPIO%d", PIN_ROT_CLK, PIN_ROT_DT);
    RotaryEncoder rotary(PIN_ROT_CLK, PIN_ROT_DT, [&led](int delta) {
        led.blink(30);
        int v = clampVolume(g_wiim->volume() + delta * VOLUME_STEP);
        ESP_LOGI(TAG, "rotary: turn  delta=%+d  volume %d -> %d",
                 delta, g_wiim->volume(), v);
        auto r = g_wiim->setVolume(v);
        logApiResult("setVolume", r);
        if (r.success) {
            g_wiim->setCachedVolume(v);
        }
    });
    rotary.begin();
    ESP_LOGI(TAG, "rotary initial CLK/DT levels: %d/%d  (any combo is fine; just noting state)",
             gpio_get_level(PIN_ROT_CLK), gpio_get_level(PIN_ROT_DT));

    ESP_LOGI(TAG, "=== ready, awaiting input ===");

    // Poll loop. 5 ms is fast enough to catch rotary turns without burning CPU.
    // While an HTTP call is in flight (a few hundred ms) we'll miss inputs --
    // good enough for v1; can be split into separate tasks if it becomes a problem.
    TickType_t lastHeartbeat = xTaskGetTickCount();
    constexpr TickType_t kHeartbeatTicks = pdMS_TO_TICKS(10000);

    while (true) {
        buttons.poll();
        rotary.poll();

        TickType_t now = xTaskGetTickCount();
        if (now - lastHeartbeat >= kHeartbeatTicks) {
            ESP_LOGI(TAG, "heartbeat: alive (free heap=%lu bytes)",
                     (unsigned long)esp_get_free_heap_size());
            lastHeartbeat = now;
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
}  // namespace

extern "C" void app_main() {
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, " WiimComs-ESP32 booting");
    ESP_LOGI(TAG, "   target URL: %s", WIIM_BASE_URL);
    ESP_LOGI(TAG, "   SSID:       %s", WIFI_SSID);
    ESP_LOGI(TAG, "===========================================");

    ESP_LOGI(TAG, "[1/4] init NVS");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "  NVS partition needed erasing");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "  -> NVS ready");

    ESP_LOGI(TAG, "[2/4] connect WiFi");
    connectWifi();
    ESP_LOGI(TAG, "  -> WiFi connected");

    ESP_LOGI(TAG, "[3/4] init HTTP client + WiimService");
    g_http = new EspHttpClient();
    g_wiim = new WiimService(g_http, WIIM_BASE_URL);
    ESP_LOGI(TAG, "  -> ready");

    ESP_LOGI(TAG, "[4/4] start wiim task");
    // 16 KB stack: mbedTLS HTTPS handshakes need ~10-15 KB during the TLS step,
    // plus DeviceStatus (~100 std::strings) lives on this task's stack as a
    // local. 8 KB overflows; 16 KB leaves comfortable headroom.
    xTaskCreatePinnedToCore(
        wiimTask,
        "wiim",
        16384,
        nullptr,
        5,
        nullptr,
        APP_CPU_NUM);
}
