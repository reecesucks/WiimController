#pragma once

#include <ArduinoJson.h>
#include <string>
#include <cstdlib>

// Mirror of the .NET PlayerStatus.
// Note: Wiim's getPlayerStatus uses lowercase keys (type, ch, mode, ...),
// while the .NET deserializer uses case-insensitive matching against PascalCase
// property names. We bind to the actual JSON keys here.
struct PlayerStatus {
    std::string type;
    std::string ch;
    std::string mode;
    std::string loop;
    std::string eq;
    std::string vendor;
    std::string status;
    std::string curpos;
    std::string offsetPts;
    std::string totlen;
    std::string title;
    std::string artist;
    std::string album;
    std::string alarmflag;
    std::string plicount;
    std::string plicurr;
    std::string vol;
    std::string mute;

    int volume = 0;

    static PlayerStatus fromJson(JsonObjectConst obj) {
        PlayerStatus p;
        p.type      = obj["type"]      | "";
        p.ch        = obj["ch"]        | "";
        p.mode      = obj["mode"]      | "";
        p.loop      = obj["loop"]      | "";
        p.eq        = obj["eq"]        | "";
        p.vendor    = obj["vendor"]    | "";
        p.status    = obj["status"]    | "";
        p.curpos    = obj["curpos"]    | "";
        p.offsetPts = obj["offset_pts"]| "";
        p.totlen    = obj["totlen"]    | "";
        p.title     = obj["Title"]     | "";
        p.artist    = obj["Artist"]    | "";
        p.album     = obj["Album"]     | "";
        p.alarmflag = obj["alarmflag"] | "";
        p.plicount  = obj["plicount"]  | "";
        p.plicurr   = obj["plicurr"]   | "";
        p.vol       = obj["vol"]       | "";
        p.mute      = obj["mute"]      | "";

        // Match .NET behavior: int.TryParse(Vol) → Volume, default 25 on failure.
        p.volume = p.vol.empty() ? 25 : std::atoi(p.vol.c_str());
        return p;
    }
};
