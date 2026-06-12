#include "WiimService.h"

#include <cctype>
#include <cstdio>
#include <cstring>

#include <ArduinoJson.h>

#include "Log.h"

namespace {
constexpr const char* TAG = "wiim";

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool iequalsAscii(const std::string& a, const char* b) {
    auto n = std::char_traits<char>::length(b);
    if (a.size() != n) return false;
    for (size_t i = 0; i < n; ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// Print up to the first 1024 chars of a response body, with a length header.
// Long enough to inspect a getStatusEx payload without flooding the log.
void logBodySnippet(const std::string& body) {
    constexpr size_t kMax = 1024;
    if (body.size() <= kMax) {
        ESP_LOGI(TAG, "  body (%u bytes): %s",
                 (unsigned)body.size(), body.c_str());
    } else {
        std::string head = body.substr(0, kMax);
        ESP_LOGI(TAG, "  body (%u bytes, showing first %u): %s ... [truncated]",
                 (unsigned)body.size(), (unsigned)kMax, head.c_str());
    }
}
}  // namespace

WiimService::WiimService(IHttpClient* http, const std::string& baseUrl)
    : _http(http), _baseUrl(baseUrl) {
    if (!_baseUrl.empty() && _baseUrl.back() != '/') {
        _baseUrl.push_back('/');
    }
}

bool WiimService::httpGet(const std::string& path, std::string& bodyOut, int& httpCodeOut) {
    if (!_http) {
        ESP_LOGE(TAG, "no IHttpClient configured");
        return false;
    }
    const std::string url = _baseUrl + path;
    ESP_LOGI(TAG, "httpGet: %s", url.c_str());
    bool ok = _http->get(url, bodyOut, httpCodeOut);
    ESP_LOGI(TAG, "httpGet: ok=%d code=%d bodyBytes=%u",
             ok ? 1 : 0, httpCodeOut, (unsigned)bodyOut.size());
    return ok;
}

WiimApiResult WiimService::makeFailed(const std::string& message) {
    WiimApiResult r;
    r.success = false;
    r.message = message;
    return r;
}

WiimApiResult WiimService::makeResult(int httpCode, const std::string& body) {
    WiimApiResult r;
    bool isOk = (httpCode >= 200 && httpCode < 300) && iequalsAscii(trim(body), "OK");
    r.success = isOk;
    r.message = body;
    return r;
}

WiimApiResult WiimService::requestSimple(const std::string& path) {
    std::string body;
    int         code = 0;
    if (!httpGet(path, body, code)) {
        return makeFailed("HTTP request failed");
    }
    return makeResult(code, body);
}

bool WiimService::setPlayerStatus() {
    PlayerStatus ps;
    if (!getPlayerStatus(ps)) return false;
    _playerStatus = ps;
    return true;
}

bool WiimService::getDeviceStatus(DeviceStatus& out) {
    ESP_LOGI(TAG, "getDeviceStatus: start");

    std::string body;
    int         code = 0;
    if (!httpGet("httpapi.asp?command=getStatusEx", body, code)) {
        ESP_LOGW(TAG, "getDeviceStatus: HTTP transport failed");
        return false;
    }
    if (code < 200 || code >= 300) {
        ESP_LOGW(TAG, "getDeviceStatus: bad HTTP status %d", code);
        logBodySnippet(body);
        return false;
    }
    logBodySnippet(body);

    ESP_LOGI(TAG, "getDeviceStatus: parsing JSON (%u bytes)...", (unsigned)body.size());
    JsonDocument doc;
    auto err = deserializeJson(doc, body);
    if (err) {
        ESP_LOGE(TAG, "getDeviceStatus: JSON parse failed: %s", err.c_str());
        return false;
    }
    ESP_LOGI(TAG, "getDeviceStatus: JSON parsed OK, populating DeviceStatus...");

    out = DeviceStatus::fromJson(doc.as<JsonObjectConst>());

    ESP_LOGI(TAG, "getDeviceStatus: done (deviceName='%s')", out.deviceName.c_str());
    return true;
}

bool WiimService::getPlayerStatus(PlayerStatus& out) {
    ESP_LOGI(TAG, "getPlayerStatus: start");

    std::string body;
    int         code = 0;
    if (!httpGet("httpapi.asp?command=getPlayerStatus", body, code)) {
        ESP_LOGW(TAG, "getPlayerStatus: HTTP transport failed");
        return false;
    }
    if (code < 200 || code >= 300) {
        ESP_LOGW(TAG, "getPlayerStatus: bad HTTP status %d", code);
        logBodySnippet(body);
        return false;
    }
    logBodySnippet(body);

    ESP_LOGI(TAG, "getPlayerStatus: parsing JSON (%u bytes)...", (unsigned)body.size());
    JsonDocument doc;
    auto err = deserializeJson(doc, body);
    if (err) {
        ESP_LOGE(TAG, "getPlayerStatus: JSON parse failed: %s", err.c_str());
        return false;
    }

    out = PlayerStatus::fromJson(doc.as<JsonObjectConst>());

    ESP_LOGI(TAG, "getPlayerStatus: done (vol=%d)", out.volume);
    return true;
}

WiimApiResult WiimService::playNextSong() {
    return requestSimple("httpapi.asp?command=setPlayerCmd:next");
}

WiimApiResult WiimService::playPreviousSong() {
    return requestSimple("httpapi.asp?command=setPlayerCmd:prev");
}

WiimApiResult WiimService::playPlaylist(int preset) {
    char path[64];
    std::snprintf(path, sizeof(path), "httpapi.asp?command=MCUKeyShortClick:%dd", preset);
    return requestSimple(path);
}

WiimApiResult WiimService::pause() {
    return requestSimple("httpapi.asp?command=setPlayerCmd:pause");
}

WiimApiResult WiimService::resume() {
    return requestSimple("httpapi.asp?command=setPlayerCmd:resume");
}

WiimApiResult WiimService::onePause() {
    return requestSimple("httpapi.asp?command=setPlayerCmd:onepause");
}

WiimApiResult WiimService::setVolume(int value) {
    char path[64];
    std::snprintf(path, sizeof(path), "httpapi.asp?command=setPlayerCmd:vol:%d", value);
    return requestSimple(path);
}

bool WiimService::getSongMetaData(MusicTrack& out) {
    ESP_LOGI(TAG, "getSongMetaData: start");

    std::string body;
    int         code = 0;
    if (!httpGet("httpapi.asp?command=getMetaInfo", body, code)) {
        ESP_LOGW(TAG, "getSongMetaData: HTTP transport failed");
        return false;
    }
    if (code < 200 || code >= 300) {
        ESP_LOGW(TAG, "getSongMetaData: bad HTTP status %d", code);
        logBodySnippet(body);
        return false;
    }
    logBodySnippet(body);

    ESP_LOGI(TAG, "getSongMetaData: parsing JSON (%u bytes)...", (unsigned)body.size());
    JsonDocument doc;
    auto err = deserializeJson(doc, body);
    if (err) {
        ESP_LOGE(TAG, "getSongMetaData: JSON parse failed: %s", err.c_str());
        return false;
    }

    out = MusicTrack::fromJson(doc.as<JsonObjectConst>());

    ESP_LOGI(TAG, "getSongMetaData: done (title='%s')", out.metaData.title.c_str());
    return true;
}
