#include "WebServer.h"
#include "esp_err.h"
#include "mdns.h"
#include "esp_http_server.h"
#include "Log.h"

namespace {
    constexpr const char* TAG = "wiim.web";

    esp_err_t rootHandler(httpd_req_t* req) {
        const char* body = "Hello, world!";
        return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
    }
}

void initMdns(const char* hostname) {
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mdns_init failed: %s", esp_err_to_name(err));
        return;
    }

    err = mdns_hostname_set(hostname);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mdns_hostname_set failed: %s", esp_err_to_name(err));
        return;
    }

    err = mdns_instance_name_set("Wiim Controller");
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "mdns_instance_name_set failed: %s", esp_err_to_name(err));
        // not fatal -- carry on
    }

    err = mdns_service_add(nullptr, "_http", "_tcp", 80, nullptr, 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "mdns_service_add failed: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "mdns ready, reachable at %s.local", hostname);
}

void startWebServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = nullptr;

    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return;
    }

    httpd_uri_t rootUri = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = rootHandler,
        .user_ctx = nullptr,
    };

    err = httpd_register_uri_handler(server, &rootUri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register / handler failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "web server listening on port %d", config.server_port);
}