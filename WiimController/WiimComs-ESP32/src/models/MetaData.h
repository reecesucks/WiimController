#pragma once

#include <ArduinoJson.h>
#include <string>

struct MetaData {
    std::string album;
    std::string title;
    std::string subtitle;
    std::string artist;
    std::string albumArtURI;
    std::string sampleRate;
    std::string bitDepth;
    std::string bitRate;
    std::string trackId;

    static MetaData fromJson(JsonObjectConst obj) {
        MetaData m;
        m.album       = obj["album"]       | "";
        m.title       = obj["title"]       | "";
        m.subtitle    = obj["subtitle"]    | "";
        m.artist      = obj["artist"]      | "";
        m.albumArtURI = obj["albumArtURI"] | "";
        m.sampleRate  = obj["sampleRate"]  | "";
        m.bitDepth    = obj["bitDepth"]    | "";
        m.bitRate     = obj["bitRate"]     | "";
        m.trackId     = obj["trackId"]     | "";
        return m;
    }
};
