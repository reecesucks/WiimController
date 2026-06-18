#pragma once

#include <string>

struct AppConfig {
    std::string wifiSsid;
    std::string wifiPassword;
    std::string wiimBaseUrl;
};

AppConfig loadConfig();
void      saveConfig(const AppConfig& cfg);