#ifndef _WIN32  // This entire file is Linux/macOS only

#include "CurlHttpClient.hpp"
#include <stdexcept>
#include <sstream>
#include <string>
#include <cctype>
#include <algorithm>
#include <mutex>

namespace net
{
    namespace
    {
        // One-time global curl initialisation.
        void ensureCurlInit()
        {
            static std::once_flag flag;
            std::call_once(flag, []() {
                curl_global_init(CURL_GLOBAL_DEFAULT);
            });
        }

        // RAII wrappers ---------------------------------------------------------

        struct CurlHandle
        {
            CURL* h;
            CurlHandle() : h(curl_easy_init()) {}
            ~CurlHandle() { if (h) curl_easy_cleanup(h); }
            CurlHandle(const CurlHandle&) = delete;
            CurlHandle& operator=(const CurlHandle&) = delete;
        };

        struct CurlSlist
        {
            curl_slist* list = nullptr;
            void append(const std::string& s) { list = curl_slist_append(list, s.c_str()); }
            ~CurlSlist() { if (list) curl_slist_free_all(list); }
        };

        // Callbacks ------------------------------------------------------------

        size_t writeBody(char* ptr, size_t size, size_t nmemb, void* userdata)
        {
            auto* body = static_cast<std::string*>(userdata);
            body->append(ptr, size * nmemb);
            return size * nmemb;
        }

        size_t writeHeader(char* buffer, size_t size, size_t nitems, void* userdata)
        {
            auto* hdrs = static_cast<std::unordered_map<std::string, std::string>*>(userdata);
            std::string line(buffer, size * nitems);

            // Strip CR/LF
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
                line.pop_back();

            auto colon = line.find(':');
            if (colon != std::string::npos)
            {
                std::string name  = line.substr(0, colon);
                std::string value = line.substr(colon + 1);
                // Trim leading whitespace from value
                auto start = value.find_first_not_of(' ');
                if (start != std::string::npos) value = value.substr(start);
                (*hdrs)[name] = value;
            }
            return size * nitems;
        }
    }

    HttpResponseData CurlHttpClient::send(const HttpRequestData& request)
    {
        ensureCurlInit();

        CurlHandle curl;
        if (!curl.h)
            throw std::runtime_error("connection:failed to initialise curl easy handle");

        HttpResponseData response;

        // URL
        curl_easy_setopt(curl.h, CURLOPT_URL, request.url.c_str());

        // Timeout
        if (request.timeoutMs > 0)
            curl_easy_setopt(curl.h, CURLOPT_TIMEOUT_MS,
                             static_cast<long>(request.timeoutMs));

        // HTTP method + body
        std::string method = request.method;
        std::transform(method.begin(), method.end(), method.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

        if (method == "POST")
        {
            curl_easy_setopt(curl.h, CURLOPT_POST, 1L);
            curl_easy_setopt(curl.h, CURLOPT_POSTFIELDS,   request.body.c_str());
            curl_easy_setopt(curl.h, CURLOPT_POSTFIELDSIZE,
                             static_cast<long>(request.body.size()));
        }
        else if (method == "PUT")
        {
            curl_easy_setopt(curl.h, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl.h, CURLOPT_POSTFIELDS,   request.body.c_str());
            curl_easy_setopt(curl.h, CURLOPT_POSTFIELDSIZE,
                             static_cast<long>(request.body.size()));
        }
        else if (method == "DELETE")
        {
            curl_easy_setopt(curl.h, CURLOPT_CUSTOMREQUEST, "DELETE");
            if (!request.body.empty())
            {
                curl_easy_setopt(curl.h, CURLOPT_POSTFIELDS,   request.body.c_str());
                curl_easy_setopt(curl.h, CURLOPT_POSTFIELDSIZE,
                                 static_cast<long>(request.body.size()));
            }
        }
        else if (method == "PATCH")
        {
            curl_easy_setopt(curl.h, CURLOPT_CUSTOMREQUEST, "PATCH");
            curl_easy_setopt(curl.h, CURLOPT_POSTFIELDS,   request.body.c_str());
            curl_easy_setopt(curl.h, CURLOPT_POSTFIELDSIZE,
                             static_cast<long>(request.body.size()));
        }
        else if (method != "GET" && method != "HEAD")
        {
            // Generic custom method
            curl_easy_setopt(curl.h, CURLOPT_CUSTOMREQUEST, request.method.c_str());
            if (!request.body.empty())
            {
                curl_easy_setopt(curl.h, CURLOPT_POSTFIELDS,   request.body.c_str());
                curl_easy_setopt(curl.h, CURLOPT_POSTFIELDSIZE,
                                 static_cast<long>(request.body.size()));
            }
        }

        // Request headers
        CurlSlist hdrs;
        for (const auto& kv : request.headers)
            hdrs.append(kv.first + ": " + kv.second);
        if (hdrs.list)
            curl_easy_setopt(curl.h, CURLOPT_HTTPHEADER, hdrs.list);

        // Response body
        curl_easy_setopt(curl.h, CURLOPT_WRITEFUNCTION, writeBody);
        curl_easy_setopt(curl.h, CURLOPT_WRITEDATA, &response.body);

        // Response headers
        curl_easy_setopt(curl.h, CURLOPT_HEADERFUNCTION, writeHeader);
        curl_easy_setopt(curl.h, CURLOPT_HEADERDATA, &response.headers);

        // Follow redirects
        curl_easy_setopt(curl.h, CURLOPT_FOLLOWLOCATION, 1L);

        // User-Agent (match WinHttpClient)
        curl_easy_setopt(curl.h, CURLOPT_USERAGENT, "mType/1.0");

        // Perform
        CURLcode res = curl_easy_perform(curl.h);
        if (res != CURLE_OK)
        {
            const char* errStr = curl_easy_strerror(res);
            if (res == CURLE_OPERATION_TIMEDOUT)
                throw std::runtime_error(std::string("timeout:") + errStr);
            if (res == CURLE_COULDNT_RESOLVE_HOST)
                throw std::runtime_error(std::string("dns:") + errStr);
            throw std::runtime_error(std::string("connection:") + errStr);
        }

        long statusCode = 0;
        curl_easy_getinfo(curl.h, CURLINFO_RESPONSE_CODE, &statusCode);
        response.status = static_cast<int>(statusCode);

        return response;
    }
}

#endif // !_WIN32
