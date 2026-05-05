#include "WinHttpClient.hpp"
#include <cstddef>

#include <windows.h>
#include <winhttp.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cwctype>

#pragma comment(lib, "winhttp.lib")

namespace net
{
    namespace
    {
        std::wstring toWide(const std::string& s)
        {
            if (s.empty()) return L"";
            int needed = MultiByteToWideChar(CP_UTF8, 0, s.data(),
                                             static_cast<int>(s.size()), nullptr, 0);
            std::wstring out(needed, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, s.data(),
                                static_cast<int>(s.size()), out.data(), needed);
            return out;
        }

        std::string toUtf8(const std::wstring& s)
        {
            if (s.empty()) return "";
            int needed = WideCharToMultiByte(CP_UTF8, 0, s.data(),
                                             static_cast<int>(s.size()),
                                             nullptr, 0, nullptr, nullptr);
            std::string out(needed, '\0');
            WideCharToMultiByte(CP_UTF8, 0, s.data(),
                                static_cast<int>(s.size()),
                                out.data(), needed, nullptr, nullptr);
            return out;
        }

        std::string lastWinHttpError(const std::string& prefix)
        {
            DWORD err = GetLastError();
            if (err == ERROR_WINHTTP_TIMEOUT)
            {
                return "timeout:" + prefix + " timed out";
            }
            if (err == ERROR_WINHTTP_NAME_NOT_RESOLVED)
            {
                return "dns:could not resolve host";
            }
            if (err == ERROR_WINHTTP_CANNOT_CONNECT ||
                err == ERROR_WINHTTP_CONNECTION_ERROR)
            {
                return "connection:" + prefix + " (WinHTTP " + std::to_string(err) + ")";
            }
            return "connection:" + prefix + " (WinHTTP error " + std::to_string(err) + ")";
        }

        struct WinHttpHandle
        {
            HINTERNET h;
            WinHttpHandle() : h(nullptr) {}
            explicit WinHttpHandle(HINTERNET handle) : h(handle) {}
            ~WinHttpHandle() { if (h) WinHttpCloseHandle(h); }
            WinHttpHandle(const WinHttpHandle&) = delete;
            WinHttpHandle& operator=(const WinHttpHandle&) = delete;
            HINTERNET get() const { return h; }
            void reset(HINTERNET handle)
            {
                if (h) WinHttpCloseHandle(h);
                h = handle;
            }
        };

        std::wstring buildHeaderBlock(const std::unordered_map<std::string, std::string>& headers)
        {
            std::wstring out;
            for (const auto& kv : headers)
            {
                out += toWide(kv.first);
                out += L": ";
                out += toWide(kv.second);
                out += L"\r\n";
            }
            return out;
        }

        std::string trim(const std::string& s)
        {
            size_t a = s.find_first_not_of(" \t\r\n");
            if (a == std::string::npos) return "";
            size_t b = s.find_last_not_of(" \t\r\n");
            return s.substr(a, b - a + 1);
        }

        std::unordered_map<std::string, std::string> parseResponseHeaders(const std::wstring& raw)
        {
            std::unordered_map<std::string, std::string> result;
            std::string utf8 = toUtf8(raw);
            std::istringstream iss(utf8);
            std::string line;
            while (std::getline(iss, line))
            {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                size_t colon = line.find(':');
                if (colon == std::string::npos) continue;
                std::string name = trim(line.substr(0, colon));
                std::string value = trim(line.substr(colon + 1));
                if (!name.empty())
                {
                    result[name] = value;
                }
            }
            return result;
        }
    }

    HttpResponseData WinHttpClient::send(const HttpRequestData& request)
    {
        std::wstring wurl = toWide(request.url);

        // Crack URL into components. Two-pass: first call with zero-length
        // buffers but non-null pointers to populate required lengths, then
        // allocate exact-size buffers. This avoids silent truncation of long
        // hostnames or paths that fixed buffers would cause.
        URL_COMPONENTS ucSizing{};
        ucSizing.dwStructSize = sizeof(ucSizing);
        ucSizing.dwHostNameLength = static_cast<DWORD>(-1);
        ucSizing.dwUrlPathLength = static_cast<DWORD>(-1);
        ucSizing.dwExtraInfoLength = static_cast<DWORD>(-1);
        ucSizing.dwUserNameLength = static_cast<DWORD>(-1);
        ucSizing.dwPasswordLength = static_cast<DWORD>(-1);
        ucSizing.dwSchemeLength = static_cast<DWORD>(-1);
        if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &ucSizing))
        {
            throw std::runtime_error("connection:malformed URL '" + request.url + "'");
        }

        std::vector<wchar_t> hostBuf(ucSizing.dwHostNameLength + 1, L'\0');
        std::vector<wchar_t> pathBuf(ucSizing.dwUrlPathLength + 1, L'\0');

        URL_COMPONENTS uc{};
        uc.dwStructSize = sizeof(uc);
        uc.lpszHostName = hostBuf.data();
        uc.dwHostNameLength = static_cast<DWORD>(hostBuf.size());
        uc.lpszUrlPath = pathBuf.data();
        uc.dwUrlPathLength = static_cast<DWORD>(pathBuf.size());

        if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc))
        {
            throw std::runtime_error("connection:malformed URL '" + request.url + "'");
        }

        bool isHttps = (uc.nScheme == INTERNET_SCHEME_HTTPS);
        INTERNET_PORT port = uc.nPort != 0 ? uc.nPort : (isHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT);

        WinHttpHandle session(WinHttpOpen(L"mType/1.0",
                                          WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                          WINHTTP_NO_PROXY_NAME,
                                          WINHTTP_NO_PROXY_BYPASS,
                                          0));
        if (!session.get())
        {
            throw std::runtime_error(lastWinHttpError("WinHttpOpen failed"));
        }

        if (request.timeoutMs > 0)
        {
            WinHttpSetTimeouts(session.get(), request.timeoutMs, request.timeoutMs,
                               request.timeoutMs, request.timeoutMs);
        }

        WinHttpHandle connect(WinHttpConnect(session.get(), hostBuf.data(), port, 0));
        if (!connect.get())
        {
            throw std::runtime_error(lastWinHttpError("WinHttpConnect failed"));
        }

        DWORD reqFlags = isHttps ? WINHTTP_FLAG_SECURE : 0;
        std::wstring wmethod = toWide(request.method);
        WinHttpHandle req(WinHttpOpenRequest(connect.get(), wmethod.c_str(), pathBuf.data(),
                                             nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, reqFlags));
        if (!req.get())
        {
            throw std::runtime_error(lastWinHttpError("WinHttpOpenRequest failed"));
        }

        std::wstring headerBlock = buildHeaderBlock(request.headers);
        if (!headerBlock.empty())
        {
            WinHttpAddRequestHeaders(req.get(), headerBlock.c_str(),
                                     static_cast<DWORD>(headerBlock.size()),
                                     WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
        }

        const void* bodyPtr = request.body.empty() ? WINHTTP_NO_REQUEST_DATA : request.body.data();
        DWORD bodyLen = static_cast<DWORD>(request.body.size());

        if (!WinHttpSendRequest(req.get(),
                                WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                const_cast<void*>(bodyPtr), bodyLen,
                                bodyLen, 0))
        {
            throw std::runtime_error(lastWinHttpError("WinHttpSendRequest failed"));
        }

        if (!WinHttpReceiveResponse(req.get(), nullptr))
        {
            throw std::runtime_error(lastWinHttpError("WinHttpReceiveResponse failed"));
        }

        HttpResponseData response;

        // Status code.
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        WinHttpQueryHeaders(req.get(),
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize,
                            WINHTTP_NO_HEADER_INDEX);
        response.status = static_cast<int>(statusCode);

        // Raw response headers.
        DWORD headerSize = 0;
        WinHttpQueryHeaders(req.get(), WINHTTP_QUERY_RAW_HEADERS_CRLF,
                            WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &headerSize,
                            WINHTTP_NO_HEADER_INDEX);
        if (headerSize > 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            std::wstring headerBuf(headerSize / sizeof(wchar_t), L'\0');
            if (WinHttpQueryHeaders(req.get(), WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                    WINHTTP_HEADER_NAME_BY_INDEX, headerBuf.data(),
                                    &headerSize, WINHTTP_NO_HEADER_INDEX))
            {
                response.headers = parseResponseHeaders(headerBuf);
            }
        }

        // Body.
        std::vector<char> bodyBuf;
        DWORD available = 0;
        do
        {
            available = 0;
            if (!WinHttpQueryDataAvailable(req.get(), &available))
            {
                throw std::runtime_error(lastWinHttpError("WinHttpQueryDataAvailable failed"));
            }
            if (available == 0) break;

            std::vector<char> chunk(available);
            DWORD read = 0;
            if (!WinHttpReadData(req.get(), chunk.data(), available, &read))
            {
                throw std::runtime_error(lastWinHttpError("WinHttpReadData failed"));
            }
            bodyBuf.insert(bodyBuf.end(), chunk.begin(), chunk.begin() + read);
        } while (available > 0);

        response.body.assign(bodyBuf.begin(), bodyBuf.end());
        return response;
    }
}
