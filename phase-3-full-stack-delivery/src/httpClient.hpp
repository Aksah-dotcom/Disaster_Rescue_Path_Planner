#pragma once
// httpClient.hpp
// JS used the browser/Node built-in fetch() with AbortController for
// timeouts. C++ has no built-in HTTP client, so this wraps libcurl —
// the de facto standard C library for making HTTP requests — with the
// same timeout + "never throw, return nullopt on any failure" contract
// that osrmClient.js and weatherClient.js both rely on.
#include <curl/curl.h>
#include <string>
#include <optional>

namespace httpClient {

inline size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    static_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// Returns std::nullopt on ANY failure (network error, timeout, non-200
// status) — callers never need to catch an exception, matching the
// JS clients' "return null on failure" contract exactly.
inline std::optional<std::string> get(const std::string& url, long timeoutMs = 6000) {
    CURL* curl = curl_easy_init();
    if (!curl) return std::nullopt;

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeoutMs);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "rescue-planner-cpp/1.0");

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || httpCode != 200) return std::nullopt;
    return response;
}

} // namespace httpClient
