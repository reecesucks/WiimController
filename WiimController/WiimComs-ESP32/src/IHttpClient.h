#pragma once

#include <string>

class IHttpClient {
public:
    virtual ~IHttpClient() = default;

    // Perform a GET against the absolute URL.
    // On success returns true and fills bodyOut + httpCodeOut (>0).
    // On transport failure returns false; httpCodeOut may be -1.
    virtual bool get(const std::string& url, std::string& bodyOut, int& httpCodeOut) = 0;
};
