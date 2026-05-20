#pragma once

#include <ArduinoJson.h>
#include <string>

struct SecurityCapabilities {
    std::string ver;
    std::string aesVer;

    static SecurityCapabilities fromJson(JsonObjectConst obj) {
        SecurityCapabilities sc;
        sc.ver    = obj["ver"]     | "";
        sc.aesVer = obj["aes_ver"] | "";
        return sc;
    }
};
