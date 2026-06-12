#pragma once

#include <string>

#include "IHttpClient.h"
#include "models/DeviceStatus.h"
#include "models/PlayerStatus.h"
#include "models/MusicTrack.h"
#include "models/WiimApiResult.h"

// Port of WiimComs.Services.WiimController.Services.WiimService.
// Methods are blocking; on ESP32 they should be called from a FreeRTOS task,
// not from app_main() directly when responsiveness matters.
//
// HTTP I/O is delegated to an IHttpClient implementation so the same logic can
// run on ESP32 (esp_http_client) and on Windows (WinHTTP) for debugging.
class WiimService {
public:
    WiimService(IHttpClient* http, const std::string& baseUrl);

    bool setPlayerStatus();
    int  volume() const { return _playerStatus.volume; }
    void setCachedVolume(int v) { _playerStatus.volume = v; }

    bool getDeviceStatus(DeviceStatus& out);
    bool getPlayerStatus(PlayerStatus& out);

    WiimApiResult playNextSong();
    WiimApiResult playPreviousSong();
    WiimApiResult playPlaylist(int preset);
    WiimApiResult pause();
    WiimApiResult resume();
    WiimApiResult onePause();
    WiimApiResult setVolume(int value);

    bool getSongMetaData(MusicTrack& out);

private:
    IHttpClient* _http;
    std::string  _baseUrl;
    PlayerStatus _playerStatus{};

    bool          httpGet(const std::string& path, std::string& bodyOut, int& httpCodeOut);
    WiimApiResult requestSimple(const std::string& path);
    static WiimApiResult makeFailed(const std::string& message);
    static WiimApiResult makeResult(int httpCode, const std::string& body);
};
