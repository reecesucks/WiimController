#include "PlaybackController.h"

#include <algorithm>   // std::min, std::max

#include "Log.h"

namespace {
constexpr const char* TAG = "wiim";

void logApiResult(const char* what, const WiimApiResult& r) {
    if (r.success) {
        ESP_LOGI(TAG, "  -> %s OK", what);
    } else {
        ESP_LOGW(TAG, "  -> %s FAILED: %s", what, r.message.c_str());
    }
}
}  // namespace

PlaybackController::PlaybackController(WiimService& wiim, int volumeStep)
    : _wiim(wiim), _volumeStep(volumeStep) {}

void PlaybackController::onVolumeChange(int delta) {
    const int current = _wiim.volume();
    const int desired = current + delta * _volumeStep;
    const int target  = std::max(kMinVolume, std::min(desired, kMaxVolume));
    ESP_LOGI(TAG, "rotary: turn  delta=%+d  volume %d -> %d", delta, current, target);

    const WiimApiResult r = _wiim.setVolume(target);
    logApiResult("setVolume", r);
    if (r.success) {
        _wiim.setCachedVolume(target);
    }
}

void PlaybackController::onPlayPause() {
    ESP_LOGI(TAG, "play/pause");
    logApiResult("onePause", _wiim.onePause());
}

void PlaybackController::onNext() {
    ESP_LOGI(TAG, "next");
    logApiResult("playNextSong", _wiim.playNextSong());
}

void PlaybackController::onPrevious() {
    ESP_LOGI(TAG, "prev");
    logApiResult("playPreviousSong", _wiim.playPreviousSong());
}
