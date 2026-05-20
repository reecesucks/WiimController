#pragma once

#include <ArduinoJson.h>
#include "MetaData.h"

struct MusicTrack {
    MetaData metaData;

    static MusicTrack fromJson(JsonObjectConst obj) {
        MusicTrack t;
        if (!obj["metaData"].isNull()) {
            t.metaData = MetaData::fromJson(obj["metaData"].as<JsonObjectConst>());
        }
        return t;
    }
};
