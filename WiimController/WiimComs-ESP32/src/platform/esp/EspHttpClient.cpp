#include "EspHttpClient.h"

#include <cstring>

#include "esp_http_client.h"
#include "esp_log.h"

namespace {
constexpr const char* TAG        = "wiim.http";
constexpr const char* kUserAgent = "WiimController/1.0";

esp_err_t httpEventHandler(esp_http_client_event_t* evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->user_data) {
        auto* body = static_cast<std::string*>(evt->user_data);
        body->append(static_cast<const char*>(evt->data), evt->data_len);
    }
    return ESP_OK;
}

bool startsWith(const std::string& s, const char* prefix) {
    auto n = std::char_traits<char>::length(prefix);
    return s.size() >= n && std::memcmp(s.data(), prefix, n) == 0;
}
}  // namespace

bool EspHttpClient::get(const std::string& url, std::string& bodyOut, int& httpCodeOut) {
    bodyOut.clear();
    httpCodeOut = -1;

    const bool useTls = startsWith(url, "https://");

    esp_http_client_config_t config = {};
    config.url                          = url.c_str();
    config.method                       = HTTP_METHOD_GET;
    config.timeout_ms                   = 5000;
    config.event_handler                = httpEventHandler;
    config.user_data                    = &bodyOut;
    config.user_agent                   = kUserAgent;
    config.transport_type               = useTls ? HTTP_TRANSPORT_OVER_SSL : HTTP_TRANSPORT_OVER_TCP;
    // Equivalent of HttpClientHandler.DangerousAcceptAnyServerCertificateValidator:
    // no CA, no bundle, skip CN check. Backed by CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY.
    config.skip_cert_common_name_check  = true;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "esp_http_client_init failed for %s", url.c_str());
        return false;
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        httpCodeOut = esp_http_client_get_status_code(client);
    } else {
        ESP_LOGW(TAG, "perform failed for %s: %s", url.c_str(), esp_err_to_name(err));
        bodyOut.clear();
    }

    esp_http_client_cleanup(client);
    return err == ESP_OK && httpCodeOut > 0;
}
