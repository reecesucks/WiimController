#pragma once

#include "IHttpClient.h"

class WinHttpClient : public IHttpClient {
public:
    WinHttpClient();
    ~WinHttpClient() override;

    WinHttpClient(const WinHttpClient&) = delete;
    WinHttpClient& operator=(const WinHttpClient&) = delete;

    bool get(const std::string& url, std::string& bodyOut, int& httpCodeOut) override;

private:
    void* _session = nullptr;  // HINTERNET; void* avoids leaking <windows.h> into the header.
};
