#include "WinSocket.hpp"
#include "WinSockInit.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>
#include <string>
#include <cstring>
#include <algorithm>
#include <climits>

#pragma comment(lib, "ws2_32.lib")

namespace net
{
    namespace
    {
        std::string lastWsaError(const std::string& prefix)
        {
            int err = WSAGetLastError();
            return prefix + " (WSA error " + std::to_string(err) + ")";
        }

        bool isTimeout(int err)
        {
            return err == WSAETIMEDOUT || err == WSAEWOULDBLOCK;
        }
    }

    WinSocket::WinSocket()
        : fd(static_cast<uintptr_t>(INVALID_SOCKET)), connected(false), timeoutMs(0)
    {
        WinSockInit::ensure();
    }

    WinSocket::WinSocket(uintptr_t acceptedFd)
        : fd(acceptedFd), connected(true), timeoutMs(0)
    {
        WinSockInit::ensure();
    }

    WinSocket::~WinSocket()
    {
        close();
    }

    void WinSocket::connect(const std::string& host, int port)
    {
        if (connected)
        {
            throw std::runtime_error("socket already connected");
        }

        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* result = nullptr;
        std::string portStr = std::to_string(port);
        int rc = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
        if (rc != 0 || !result)
        {
            throw std::runtime_error("dns:could not resolve host '" + host + "'");
        }

        SOCKET s = INVALID_SOCKET;
        std::string lastErr;
        for (addrinfo* ai = result; ai != nullptr; ai = ai->ai_next)
        {
            s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if (s == INVALID_SOCKET)
            {
                lastErr = lastWsaError("socket() failed");
                continue;
            }

            if (timeoutMs > 0)
            {
                DWORD t = static_cast<DWORD>(timeoutMs);
                setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&t), sizeof(t));
                setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&t), sizeof(t));
            }

            if (::connect(s, ai->ai_addr, static_cast<int>(ai->ai_addrlen)) == 0)
            {
                fd = static_cast<uintptr_t>(s);
                connected = true;
                freeaddrinfo(result);
                return;
            }

            int err = WSAGetLastError();
            if (isTimeout(err))
            {
                lastErr = "timeout:connection timed out to " + host + ":" + std::to_string(port);
            }
            else
            {
                lastErr = "connection:could not connect to " + host + ":" + std::to_string(port) +
                          " (WSA error " + std::to_string(err) + ")";
            }
            closesocket(s);
            s = INVALID_SOCKET;
        }

        freeaddrinfo(result);
        throw std::runtime_error(lastErr.empty() ? "connection:unknown error" : lastErr);
    }

    int WinSocket::send(const std::string& data)
    {
        if (!connected)
        {
            throw std::runtime_error("connection:socket not connected");
        }
        SOCKET s = static_cast<SOCKET>(fd);
        size_t total = 0;
        const char* buf = data.data();
        size_t remaining = data.size();
        while (remaining > 0)
        {
            // Clamp each send() call to INT_MAX so payloads larger than 2GB
            // are transmitted in multiple chunks rather than truncated.
            int chunk = remaining > static_cast<size_t>(INT_MAX)
                ? INT_MAX
                : static_cast<int>(remaining);
            int sent = ::send(s, buf + total, chunk, 0);
            if (sent == SOCKET_ERROR)
            {
                int err = WSAGetLastError();
                if (isTimeout(err))
                {
                    throw std::runtime_error("timeout:send timed out");
                }
                throw std::runtime_error(lastWsaError("send failed"));
            }
            total += static_cast<size_t>(sent);
            remaining -= static_cast<size_t>(sent);
        }
        return total > static_cast<size_t>(INT_MAX) ? INT_MAX : static_cast<int>(total);
    }

    std::string WinSocket::recv(int maxBytes)
    {
        auto bytes = recvBytes(maxBytes);
        return std::string(bytes.begin(), bytes.end());
    }

    std::vector<uint8_t> WinSocket::recvBytes(int maxBytes)
    {
        if (!connected)
        {
            throw std::runtime_error("connection:socket not connected");
        }
        if (maxBytes <= 0)
        {
            return {};
        }
        SOCKET s = static_cast<SOCKET>(fd);
        std::vector<uint8_t> buffer(static_cast<size_t>(maxBytes));
        int got = ::recv(s, reinterpret_cast<char*>(buffer.data()), maxBytes, 0);
        if (got == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (isTimeout(err))
            {
                throw std::runtime_error("timeout:recv timed out");
            }
            throw std::runtime_error(lastWsaError("recv failed"));
        }
        buffer.resize(static_cast<size_t>(got));
        return buffer;
    }

    void WinSocket::close()
    {
        if (fd != static_cast<uintptr_t>(INVALID_SOCKET))
        {
            closesocket(static_cast<SOCKET>(fd));
            fd = static_cast<uintptr_t>(INVALID_SOCKET);
        }
        connected = false;
    }

    bool WinSocket::isConnected() const
    {
        return connected;
    }

    void WinSocket::setTimeout(int ms)
    {
        timeoutMs = ms;
        if (fd != static_cast<uintptr_t>(INVALID_SOCKET))
        {
            SOCKET s = static_cast<SOCKET>(fd);
            DWORD t = static_cast<DWORD>(ms);
            setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&t), sizeof(t));
            setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&t), sizeof(t));
        }
    }

    // ============================================================
    // WinSocketServer
    // ============================================================

    WinSocketServer::WinSocketServer()
        : listenFd(static_cast<uintptr_t>(INVALID_SOCKET)), stopping(false)
    {
        WinSockInit::ensure();
    }

    WinSocketServer::~WinSocketServer()
    {
        stop();
    }

    void WinSocketServer::start(int port,
                                std::function<void(uintptr_t)> onAccept,
                                std::function<void(const std::string&)> onError)
    {
        std::lock_guard<std::mutex> lock(stateMutex);

        if (listenFd.load() != static_cast<uintptr_t>(INVALID_SOCKET))
        {
            throw std::runtime_error("server already listening");
        }

        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET)
        {
            throw std::runtime_error(lastWsaError("server socket() failed"));
        }

        int yes = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            closesocket(s);
            throw std::runtime_error("connection:bind failed on port " + std::to_string(port) +
                                     " (WSA error " + std::to_string(err) + ")");
        }

        if (listen(s, SOMAXCONN) == SOCKET_ERROR)
        {
            closesocket(s);
            throw std::runtime_error(lastWsaError("listen failed"));
        }

        listenFd.store(static_cast<uintptr_t>(s));
        stopping = false;

        acceptThread = std::thread([this, onAccept, onError]() {
            SOCKET ls = static_cast<SOCKET>(listenFd.load());
            while (!stopping.load())
            {
                sockaddr_in clientAddr{};
                int addrLen = sizeof(clientAddr);
                SOCKET client = accept(ls, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
                if (client == INVALID_SOCKET)
                {
                    if (stopping.load())
                    {
                        break;
                    }
                    if (onError)
                    {
                        onError(lastWsaError("accept failed"));
                    }
                    continue;
                }
                if (onAccept)
                {
                    onAccept(static_cast<uintptr_t>(client));
                }
            }
        });
    }

    void WinSocketServer::stop()
    {
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            uintptr_t fd = listenFd.load();
            if (fd == static_cast<uintptr_t>(INVALID_SOCKET) && !acceptThread.joinable())
            {
                return;
            }
            stopping = true;
            if (fd != static_cast<uintptr_t>(INVALID_SOCKET))
            {
                // Close listen FD to unblock accept().
                closesocket(static_cast<SOCKET>(fd));
                listenFd.store(static_cast<uintptr_t>(INVALID_SOCKET));
            }
        }
        if (acceptThread.joinable())
        {
            acceptThread.join();
        }
    }
}
