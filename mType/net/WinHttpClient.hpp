#pragma once
#include "HttpClient.hpp"

namespace net
{
    class WinHttpClient : public HttpClient
    {
    public:
        WinHttpClient() = default;
        ~WinHttpClient() override = default;

        HttpResponseData send(const HttpRequestData& request) override;
    };
}
