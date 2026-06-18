#include "WebServer.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <ArduinoJson.h>

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "mdns.h"

#include "esp_timer.h"
#include "esp_system.h"

#include "Config.h"
#include "EspHttpClient.h"
#include "Log.h"

namespace {
    constexpr const char* TAG = "wiim.web";

    const char* HTML_INDEX = R"HTML(<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Wiim Controller</title>
    <style>
        body   { font-family: sans-serif; max-width: 480px; margin: 2em auto; padding: 0 1em; }
        label  { display: block; margin: 1em 0; font-weight: bold; }
        input  { display: block; width: 100%; padding: 0.5em; margin-top: 0.25em;
                 box-sizing: border-box; font-family: inherit; }
        button { padding: 0.75em 1.5em; font-size: 1em; cursor: pointer; }
        button.test { padding: 0.4em 1em; font-size: 0.9em; background: #eee;
                      margin-top: 0.5em; }
        button.primary { background: #06c; color: white; border: none; border-radius: 4px;
                         margin-top: 1em; }
        button.primary:hover { background: #048; }
        .result { font-family: monospace; font-size: 0.9em; margin-top: 0.5em;
                  padding: 0.4em 0; min-height: 1.2em; }
        .result.ok  { color: #060; }
        .result.err { color: #c00; }
        hr { margin: 2em 0; border: none; border-top: 1px solid #ccc; }
    </style>
</head>
<body>
    <h1>Wiim Controller</h1>

    <label>WiFi SSID
        <input type="text" id="ssid" name="ssid">
    </label>

    <label>WiFi Password
        <input type="password" id="password" name="password">
    </label>

    <button type="button" class="test" onclick="testWifi()">Test WiFi</button>
    <div id="wifi-result" class="result"></div>

    <label>Wiim Base URL
        <input type="text" id="base_url" name="base_url" placeholder="https://192.168.0.116">
    </label>

    <button type="button" class="test" onclick="testWiim()">Test Wiim</button>
    <div id="wiim-result" class="result"></div>

    <hr>

    <button type="button" class="primary" onclick="saveConfig()">Save and Restart</button>
    <div id="save-result" class="result"></div>

    <hr>

    <a href="/logs"><button type="button">View Logs</button></a>

    <script>
        async function postTest(endpoint, body, resultId) {
            const result = document.getElementById(resultId);
            result.className = 'result';
            result.textContent = 'testing...';
            try {
                const response = await fetch(endpoint, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(body)
                });
                const text = await response.text();
                result.className = 'result ' + (response.ok ? 'ok' : 'err');
                result.textContent = (response.ok ? 'OK: ' : 'FAIL: ') + text;
            } catch (e) {
                result.className = 'result err';
                result.textContent = 'FAIL: ' + e.message;
            }
        }

        function testWifi() {
            postTest('/api/test-wifi', {
                ssid:     document.getElementById('ssid').value,
                password: document.getElementById('password').value
            }, 'wifi-result');
        }

        function testWiim() {
            postTest('/api/test-wiim', {
                base_url: document.getElementById('base_url').value
            }, 'wiim-result');
        }

        async function saveConfig() {
            if (!confirm('Save and restart the device? It will be offline for ~10 seconds.')) {
                return;
            }
            const result = document.getElementById('save-result');
            result.className = 'result';
            result.textContent = 'saving...';

            const body = {
                ssid:     document.getElementById('ssid').value,
                password: document.getElementById('password').value,
                base_url: document.getElementById('base_url').value
            };

            try {
                const response = await fetch('/api/save', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(body)
                });
                const text = await response.text();
                if (response.ok) {
                    result.className = 'result ok';
                    result.textContent = 'OK: ' + text + ' (auto-reload in 15s)';
                    setTimeout(() => location.reload(), 15000);
                } else {
                    result.className = 'result err';
                    result.textContent = 'FAIL: ' + text;
                }
            } catch (e) {
                result.className = 'result err';
                result.textContent = 'FAIL: ' + e.message;
            }
        }
    </script>
</body>
</html>
)HTML";

    const char* HTML_LOGS = R"HTML(<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Logs - Wiim Controller</title>
    <style>
        body { font-family: monospace; padding: 1em; }
        pre  { background: #f0f0f0; padding: 1em; white-space: pre-wrap;
               border: 1px solid #ccc; border-radius: 4px; }
        a    { font-family: sans-serif; }
    </style>
</head>
<body>
    <p><a href="/">&larr; Back</a></p>
    <h1>Logs</h1>
    <pre>(no logs yet -- coming in a later step)</pre>
</body>
</html>
)HTML";

    esp_err_t rootHandler(httpd_req_t* req) {
        httpd_resp_set_type(req, "text/html");
        return httpd_resp_send(req, HTML_INDEX, HTTPD_RESP_USE_STRLEN);
    }

    esp_err_t logsHandler(httpd_req_t* req) {
        httpd_resp_set_type(req, "text/html");
        return httpd_resp_send(req, HTML_LOGS, HTTPD_RESP_USE_STRLEN);
    }

    // --- Helpers for the test endpoints -----------------------------------

    // Read the entire POST body and parse it as JSON into `doc`.
    // Returns true on success; on failure, sends a 400 response and returns false.
    bool readJsonBody(httpd_req_t* req, JsonDocument& doc) {
        constexpr size_t MAX_BODY = 512;
        if (req->content_len == 0 || req->content_len > MAX_BODY) {
            httpd_resp_set_status(req, "400 Bad Request");
            httpd_resp_set_type(req, "text/plain");
            httpd_resp_send(req, "body too large or empty", HTTPD_RESP_USE_STRLEN);
            return false;
        }
        char buf[MAX_BODY + 1];
        int len = httpd_req_recv(req, buf, req->content_len);
        if (len <= 0) {
            return false;
        }
        buf[len] = '\0';
        auto err = deserializeJson(doc, buf);
        if (err) {
            ESP_LOGW(TAG, "JSON parse failed: %s", err.c_str());
            httpd_resp_set_status(req, "400 Bad Request");
            httpd_resp_set_type(req, "text/plain");
            httpd_resp_send(req, "invalid JSON body", HTTPD_RESP_USE_STRLEN);
            return false;
        }
        return true;
    }

    // Send a plain-text response with the given status line. The JS in
    // HTML_INDEX uses `response.ok` (true for 2xx) and `response.text()`.
    esp_err_t sendText(httpd_req_t* req, const char* status, const char* message) {
        httpd_resp_set_status(req, status);
        httpd_resp_set_type(req, "text/plain");
        return httpd_resp_send(req, message, HTTPD_RESP_USE_STRLEN);
    }

    // --- Test handlers ----------------------------------------------------

    esp_err_t testWifiHandler(httpd_req_t* req) {
        ESP_LOGI(TAG, "POST /api/test-wifi");

        JsonDocument doc;
        if (!readJsonBody(req, doc)) return ESP_OK;  // helper sent response already

        std::string ssid     = doc["ssid"]     | "";
        std::string password = doc["password"] | "";

        // 1. Basic format checks.
        if (ssid.empty()) {
            return sendText(req, "400 Bad Request", "SSID is empty");
        }
        if (!password.empty() && password.length() < 8) {
            return sendText(req, "400 Bad Request",
                            "password too short for WPA2 (must be empty or 8+ chars)");
        }

        // 2. Scan the airwaves and check the SSID is visible.
        wifi_scan_config_t scanCfg = {};
        scanCfg.show_hidden = true;
        ESP_LOGI(TAG, "starting WiFi scan...");
        esp_err_t err = esp_wifi_scan_start(&scanCfg, true);  // true = blocking
        if (err != ESP_OK) {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "scan failed: %s", esp_err_to_name(err));
            return sendText(req, "500 Internal Server Error", buf);
        }

        uint16_t apCount = 0;
        esp_wifi_scan_get_ap_num(&apCount);
        ESP_LOGI(TAG, "scan found %u networks", apCount);

        if (apCount == 0) {
            return sendText(req, "400 Bad Request", "no WiFi networks visible");
        }

        std::vector<wifi_ap_record_t> records(apCount);
        esp_wifi_scan_get_ap_records(&apCount, records.data());

        bool found = false;
        for (uint16_t i = 0; i < apCount; ++i) {
            if (ssid == reinterpret_cast<const char*>(records[i].ssid)) {
                found = true;
                break;
            }
        }

        if (!found) {
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                          "SSID '%s' not found in %u visible networks",
                          ssid.c_str(), apCount);
            return sendText(req, "400 Bad Request", buf);
        }

        // 3. Confirm the device has internet (via the *current* connection,
        //    not the typed creds -- that's the option A trade-off).
        EspHttpClient http;
        std::string body;
        int code = 0;
        bool ok = http.get("http://connectivitycheck.gstatic.com/generate_204",
                           body, code);
        if (!ok) {
            return sendText(req, "502 Bad Gateway",
                            "SSID visible, but device has no internet");
        }
        if (code != 204 && code != 200) {
            char buf[120];
            std::snprintf(buf, sizeof(buf),
                          "SSID visible, but connectivity check returned HTTP %d", code);
            return sendText(req, "502 Bad Gateway", buf);
        }

        char buf[128];
        std::snprintf(buf, sizeof(buf),
                      "SSID '%s' visible, internet OK (note: password not verified -- click Save to apply)",
                      ssid.c_str());
        return sendText(req, "200 OK", buf);
    }

    // Schedule a device reboot ~1s in the future so the HTTP response has
    // time to be flushed back to the browser before the chip resets.
    void scheduleRestart(uint64_t delayUs = 1'000'000) {
        static esp_timer_handle_t restartTimer = nullptr;
        if (!restartTimer) {
            esp_timer_create_args_t args = {};
            args.callback = [](void*) { esp_restart(); };
            args.name     = "save-restart";
            esp_timer_create(&args, &restartTimer);
        }
        esp_timer_stop(restartTimer);  // in case it's already armed
        esp_timer_start_once(restartTimer, delayUs);
    }

    esp_err_t saveConfigHandler(httpd_req_t* req) {
        ESP_LOGI(TAG, "POST /api/save");

        JsonDocument doc;
        if (!readJsonBody(req, doc)) return ESP_OK;

        std::string ssid     = doc["ssid"]     | "";
        std::string password = doc["password"] | "";
        std::string baseUrl  = doc["base_url"] | "";

        // Load the current config so blank fields fall back to existing values --
        // lets the user update just one field without retyping the others.
        AppConfig cfg = loadConfig();
        if (!ssid.empty())     cfg.wifiSsid     = ssid;
        if (!password.empty()) cfg.wifiPassword = password;
        if (!baseUrl.empty())  cfg.wiimBaseUrl  = baseUrl;

        if (cfg.wifiSsid.empty()) {
            return sendText(req, "400 Bad Request", "SSID cannot be empty");
        }
        if (!cfg.wifiPassword.empty() && cfg.wifiPassword.length() < 8) {
            return sendText(req, "400 Bad Request",
                            "password must be empty or 8+ chars");
        }
        if (cfg.wiimBaseUrl.empty()) {
            return sendText(req, "400 Bad Request", "Wiim Base URL cannot be empty");
        }

        ESP_LOGI(TAG, "saving config: ssid='%s' url='%s' (password hidden)",
                 cfg.wifiSsid.c_str(), cfg.wiimBaseUrl.c_str());
        saveConfig(cfg);

        esp_err_t r = sendText(req, "200 OK", "saved -- restarting now");
        scheduleRestart();
        return r;
    }

    esp_err_t testWiimHandler(httpd_req_t* req) {
        ESP_LOGI(TAG, "POST /api/test-wiim");

        JsonDocument doc;
        if (!readJsonBody(req, doc)) return ESP_OK;

        std::string baseUrl = doc["base_url"] | "";
        if (baseUrl.empty()) {
            return sendText(req, "400 Bad Request", "base_url is empty");
        }

        // Normalise: strip trailing slashes.
        while (!baseUrl.empty() && baseUrl.back() == '/') baseUrl.pop_back();

        std::string url = baseUrl + "/httpapi.asp?command=getStatusEx";
        ESP_LOGI(TAG, "test-wiim: GET %s", url.c_str());

        EspHttpClient http;
        std::string body;
        int code = 0;
        bool ok = http.get(url, body, code);

        if (!ok) {
            ESP_LOGW(TAG, "test-wiim: transport failure for %s", url.c_str());
            char buf[320];
            std::snprintf(buf, sizeof(buf),
                          "could not reach Wiim at %s (network/TLS error)",
                          url.c_str());
            return sendText(req, "502 Bad Gateway", buf);
        }
        if (code < 200 || code >= 300) {
            ESP_LOGW(TAG, "test-wiim: HTTP %d from %s", code, url.c_str());
            char buf[320];
            std::snprintf(buf, sizeof(buf),
                          "Wiim at %s returned HTTP %d", url.c_str(), code);
            return sendText(req, "502 Bad Gateway", buf);
        }

        JsonDocument respDoc;
        auto err = deserializeJson(respDoc, body);
        if (err) {
            return sendText(req, "502 Bad Gateway", "Wiim response wasn't valid JSON");
        }

        std::string deviceName = respDoc["DeviceName"] | "";
        char buf[160];
        if (deviceName.empty()) {
            std::snprintf(buf, sizeof(buf),
                          "got response (%u bytes) but no DeviceName field",
                          (unsigned)body.size());
        } else {
            std::snprintf(buf, sizeof(buf), "connected to '%s'", deviceName.c_str());
        }
        return sendText(req, "200 OK", buf);
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

    httpd_uri_t logsUri = {
        .uri      = "/logs",
        .method   = HTTP_GET,
        .handler  = logsHandler,
        .user_ctx = nullptr,
    };

    err = httpd_register_uri_handler(server, &logsUri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register /logs handler failed: %s", esp_err_to_name(err));
        return;
    }

    httpd_uri_t testWifiUri = {
        .uri      = "/api/test-wifi",
        .method   = HTTP_POST,
        .handler  = testWifiHandler,
        .user_ctx = nullptr,
    };

    err = httpd_register_uri_handler(server, &testWifiUri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register /api/test-wifi handler failed: %s", esp_err_to_name(err));
        return;
    }

    httpd_uri_t testWiimUri = {
        .uri      = "/api/test-wiim",
        .method   = HTTP_POST,
        .handler  = testWiimHandler,
        .user_ctx = nullptr,
    };

    err = httpd_register_uri_handler(server, &testWiimUri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register /api/test-wiim handler failed: %s", esp_err_to_name(err));
        return;
    }

    httpd_uri_t saveUri = {
        .uri      = "/api/save",
        .method   = HTTP_POST,
        .handler  = saveConfigHandler,
        .user_ctx = nullptr,
    };

    err = httpd_register_uri_handler(server, &saveUri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register /api/save handler failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "web server listening on port %d", config.server_port);
}