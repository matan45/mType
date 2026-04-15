#pragma once
#include <string>
#include <unordered_map>

namespace net
{
    struct HttpRequestData
    {
        std::string method;
        std::string url;
        std::string body;
        std::unordered_map<std::string, std::string> headers;
        int timeoutMs = 30000;
    };

    struct HttpResponseData
    {
        int status = 0;
        std::string body;
        std::unordered_map<std::string, std::string> headers;
    };

    class HttpClient
    {
    public:
        virtual ~HttpClient() = default;

        // Synchronous request. Throws std::runtime_error on transport / DNS / timeout
        // errors with prefix "dns:", "timeout:", or "connection:" so NetErrors can map
        // them to the appropriate mType exception class.
        virtual HttpResponseData send(const HttpRequestData& request) = 0;
    };
}
