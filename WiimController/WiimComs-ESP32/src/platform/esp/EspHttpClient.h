#pragma once

#include "IHttpClient.h"

class EspHttpClient : public IHttpClient {
public:
    bool get(const std::string& url, std::string& bodyOut, int& httpCodeOut) override;
};
