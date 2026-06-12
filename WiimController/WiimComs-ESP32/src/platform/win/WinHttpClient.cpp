#include "WinHttpClient.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

#include <vector>

#include "Log.h"

#pragma comment(lib, "winhttp.lib")

namespace {
constexpr const char* TAG = "wiim.http";

std::wstring toWide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) return {};
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}
}  // namespace

WinHttpClient::WinHttpClient() {
    _session = WinHttpOpen(L"WiimController/1.0",
                           WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS,
                           0);
    if (!_session) {
        ESP_LOGE(TAG, "WinHttpOpen failed: %lu", GetLastError());
    }
}

WinHttpClient::~WinHttpClient() {
    if (_session) {
        WinHttpCloseHandle(static_cast<HINTERNET>(_session));
        _session = nullptr;
    }
}

bool WinHttpClient::get(const std::string& url, std::string& bodyOut, int& httpCodeOut) {
    bodyOut.clear();
    httpCodeOut = -1;

    if (!_session) return false;

    ESP_LOGI(TAG, "GET %s", url.c_str());

    std::wstring wurl = toWide(url);

    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {0};
    wchar_t path[2048] = {0};
    uc.lpszHostName     = host;
    uc.dwHostNameLength = sizeof(host) / sizeof(host[0]);
    uc.lpszUrlPath      = path;
    uc.dwUrlPathLength  = sizeof(path) / sizeof(path[0]);

    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) {
        ESP_LOGE(TAG, "WinHttpCrackUrl failed for %s: %lu", url.c_str(), GetLastError());
        return false;
    }

    HINTERNET conn = WinHttpConnect(static_cast<HINTERNET>(_session), host, uc.nPort, 0);
    if (!conn) {
        ESP_LOGE(TAG, "WinHttpConnect failed for %s: %lu", url.c_str(), GetLastError());
        return false;
    }

    DWORD reqFlags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET req = WinHttpOpenRequest(conn,
                                       L"GET",
                                       path,
                                       nullptr,
                                       WINHTTP_NO_REFERER,
                                       WINHTTP_DEFAULT_ACCEPT_TYPES,
                                       reqFlags);
    if (!req) {
        ESP_LOGE(TAG, "WinHttpOpenRequest failed for %s: %lu", url.c_str(), GetLastError());
        WinHttpCloseHandle(conn);
        return false;
    }

    if (reqFlags & WINHTTP_FLAG_SECURE) {
        // Match the ESP build's CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY behaviour:
        // accept self-signed and CN-mismatched certs the Wiim presents.
        DWORD security = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                         SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
        WinHttpSetOption(req, WINHTTP_OPTION_SECURITY_FLAGS, &security, sizeof(security));

        // The Wiim's TLS handshake requests a client certificate. Tell WinHTTP
        // explicitly that we have none, otherwise WinHttpReceiveResponse fails
        // with ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED (12044).
        WinHttpSetOption(req,
                         WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
                         WINHTTP_NO_CLIENT_CERT_CONTEXT,
                         0);
    }

    DWORD timeoutMs = 5000;
    WinHttpSetTimeouts(req, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    bool ok = WinHttpSendRequest(req,
                                 WINHTTP_NO_ADDITIONAL_HEADERS,
                                 0,
                                 WINHTTP_NO_REQUEST_DATA,
                                 0,
                                 0,
                                 0) &&
              WinHttpReceiveResponse(req, nullptr);

    if (ok) {
        DWORD status     = 0;
        DWORD statusSize = sizeof(status);
        WinHttpQueryHeaders(req,
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX,
                            &status,
                            &statusSize,
                            WINHTTP_NO_HEADER_INDEX);
        httpCodeOut = static_cast<int>(status);

        DWORD avail = 0;
        while (WinHttpQueryDataAvailable(req, &avail) && avail > 0) {
            std::vector<char> buf(avail);
            DWORD read = 0;
            if (!WinHttpReadData(req, buf.data(), avail, &read) || read == 0) break;
            bodyOut.append(buf.data(), read);
        }
        ESP_LOGI(TAG, "  -> HTTP %d (%u bytes)", httpCodeOut, (unsigned)bodyOut.size());
    } else {
        ESP_LOGW(TAG, "request failed for %s: %lu", url.c_str(), GetLastError());
    }

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(conn);
    return ok && httpCodeOut > 0;
}
