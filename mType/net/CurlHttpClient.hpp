#pragma once
#include "HttpClient.hpp"

namespace net
{
    // libcurl-backed HTTP/HTTPS client for Linux and macOS.
    class CurlHttpClient : public HttpClient
    {
    public:
        CurlHttpClient() = default;
        ~CurlHttpClient() override = default;

        HttpResponseData send(const HttpRequestData& request) override;
    };
}
