#pragma once

#include "WiimService.h"

class PlaybackController {
public:
    // volumeStep is the percent change per rotary detent.
    PlaybackController(WiimService& wiim, int volumeStep = 5);

    void onVolumeChange(int delta);
    void onPlayPause();
    void onNext();
    void onPrevious();

private:
    static constexpr int kMinVolume = 0;
    static constexpr int kMaxVolume = 100;

    WiimService& _wiim;
    int          _volumeStep;
};
