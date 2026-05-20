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
    return _http->get(_baseUrl + path, bodyOut, httpCodeOut);
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
    std::string body;
    int         code = 0;
    if (!httpGet("httpapi.asp?command=getStatusEx", body, code) || code < 200 || code >= 300) {
        return false;
    }

    JsonDocument doc;
    auto err = deserializeJson(doc, body);
    if (err) return false;

    out = DeviceStatus::fromJson(doc.as<JsonObjectConst>());
    return true;
}

bool WiimService::getPlayerStatus(PlayerStatus& out) {
    std::string body;
    int         code = 0;
    if (!httpGet("httpapi.asp?command=getPlayerStatus", body, code) || code < 200 || code >= 300) {
        return false;
    }

    JsonDocument doc;
    auto err = deserializeJson(doc, body);
    if (err) return false;

    out = PlayerStatus::fromJson(doc.as<JsonObjectConst>());
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
    std::string body;
    int         code = 0;
    if (!httpGet("httpapi.asp?command=getMetaInfo", body, code) || code < 200 || code >= 300) {
        return false;
    }

    JsonDocument doc;
    auto err = deserializeJson(doc, body);
    if (err) return false;

    out = MusicTrack::fromJson(doc.as<JsonObjectConst>());
    return true;
}
