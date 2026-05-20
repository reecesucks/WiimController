#include <cstdio>

#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #include "secrets.example.h"
  #pragma message("Using secrets.example.h placeholders. Copy include/secrets.example.h to include/secrets.h.")
#endif

#include "WiimService.h"
#include "platform/win/WinHttpClient.h"

int main() {
    std::printf("WiimComs Windows debug runner\n");
    std::printf("base URL: %s\n", WIIM_BASE_URL);

    WinHttpClient http;
    WiimService   wiim(&http, WIIM_BASE_URL);

    DeviceStatus ds;
    if (wiim.getDeviceStatus(ds)) {
        std::printf("device: %s (firmware %s, MAC %s)\n",
                    ds.deviceName.c_str(),
                    ds.firmware.c_str(),
                    ds.mac.c_str());
    } else {
        std::printf("getDeviceStatus failed\n");
    }

    if (wiim.setPlayerStatus()) {
        std::printf("volume=%d\n", wiim.volume());
    } else {
        std::printf("setPlayerStatus failed\n");
    }

    MusicTrack track;
    if (wiim.getSongMetaData(track)) {
        std::printf("now playing: %s - %s (%s)\n",
                    track.metaData.artist.c_str(),
                    track.metaData.title.c_str(),
                    track.metaData.album.c_str());
    } else {
        std::printf("getSongMetaData failed\n");
    }

    return 0;
}
