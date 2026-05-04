#ifndef _WIN32  // This entire file is Linux/macOS only

#include "PosixSocket.hpp"

#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <climits>

namespace net
{
    namespace
    {
        std::string posixError(const std::string& prefix)
        {
            return prefix + " (" + std::strerror(errno) + ")";
        }

        bool isTimeoutErrno(int err)
        {
            return err == EAGAIN || err == EWOULDBLOCK || err == ETIMEDOUT;
        }

        void applySocketTimeout(int fd, int ms)
        {
            if (ms <= 0 || fd < 0) return;
            struct timeval tv;
            tv.tv_sec  = ms / 1000;
            tv.tv_usec = (ms % 1000) * 1000;
            setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                       reinterpret_cast<const char*>(&tv), sizeof(tv));
            setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO,
                       reinterpret_cast<const char*>(&tv), sizeof(tv));
        }
    }

    PosixSocket::PosixSocket()
        : fd(-1), connected(false), timeoutMs(0)
    {
    }

    PosixSocket::PosixSocket(int acceptedFd)
        : fd(acceptedFd), connected(true), timeoutMs(0)
    {
    }

    PosixSocket::~PosixSocket()
    {
        close();
    }

    void PosixSocket::connect(const std::string& host, int port)
    {
        if (connected)
            throw std::runtime_error("socket already connected");

        addrinfo hints{};
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* result = nullptr;
        std::string portStr = std::to_string(port);
        int rc = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
        if (rc != 0 || !result)
            throw std::runtime_error("dns:could not resolve host '" + host + "'");

        int s = -1;
        std::string lastErr;
        for (addrinfo* ai = result; ai != nullptr; ai = ai->ai_next)
        {
            s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if (s < 0)
            {
                lastErr = posixError("socket() failed");
                continue;
            }

            if (timeoutMs > 0)
                applySocketTimeout(s, timeoutMs);

            if (::connect(s, ai->ai_addr, static_cast<socklen_t>(ai->ai_addrlen)) == 0)
            {
                fd        = s;
                connected = true;
                freeaddrinfo(result);
                return;
            }

            int err = errno;
            if (isTimeoutErrno(err))
                lastErr = "timeout:connection timed out to " + host +
                          ":" + std::to_string(port);
            else
                lastErr = "connection:could not connect to " + host +
                          ":" + std::to_string(port) +
                          " (" + std::strerror(err) + ")";

            ::close(s);
            s = -1;
        }

        freeaddrinfo(result);
        throw std::runtime_error(lastErr.empty() ? "connection:unknown error" : lastErr);
    }

    int PosixSocket::send(const std::string& data)
    {
        if (!connected)
            throw std::runtime_error("connection:socket not connected");

        size_t total     = 0;
        const char* buf  = data.data();
        size_t remaining = data.size();

        while (remaining > 0)
        {
            int chunk = remaining > static_cast<size_t>(INT_MAX)
                ? INT_MAX
                : static_cast<int>(remaining);

            ssize_t sent = ::send(fd, buf + total, static_cast<size_t>(chunk), 0);
            if (sent < 0)
            {
                int err = errno;
                if (isTimeoutErrno(err))
                    throw std::runtime_error("timeout:send timed out");
                throw std::runtime_error(posixError("send failed"));
            }
            total     += static_cast<size_t>(sent);
            remaining -= static_cast<size_t>(sent);
        }
        return total > static_cast<size_t>(INT_MAX)
            ? INT_MAX
            : static_cast<int>(total);
    }

    std::string PosixSocket::recv(int maxBytes)
    {
        auto bytes = recvBytes(maxBytes);
        return std::string(bytes.begin(), bytes.end());
    }

    std::vector<uint8_t> PosixSocket::recvBytes(int maxBytes)
    {
        if (!connected)
            throw std::runtime_error("connection:socket not connected");
        if (maxBytes <= 0)
            return {};

        std::vector<uint8_t> buffer(static_cast<size_t>(maxBytes));
        ssize_t got = ::recv(fd,
                             reinterpret_cast<char*>(buffer.data()),
                             static_cast<size_t>(maxBytes),
                             0);
        if (got < 0)
        {
            int err = errno;
            if (isTimeoutErrno(err))
                throw std::runtime_error("timeout:recv timed out");
            throw std::runtime_error(posixError("recv failed"));
        }
        buffer.resize(static_cast<size_t>(got));
        return buffer;
    }

    void PosixSocket::close()
    {
        if (fd >= 0)
        {
            ::close(fd);
            fd = -1;
        }
        connected = false;
    }

    bool PosixSocket::isConnected() const
    {
        return connected;
    }

    void PosixSocket::setTimeout(int ms)
    {
        timeoutMs = ms;
        if (fd >= 0)
            applySocketTimeout(fd, ms);
    }

    // ============================================================
    // PosixSocketServer
    // ============================================================

    PosixSocketServer::PosixSocketServer()
        : listenFd(-1), stopping(false)
    {
    }

    PosixSocketServer::~PosixSocketServer()
    {
        stop();
    }

    void PosixSocketServer::start(int port,
                                  std::function<void(uintptr_t)> onAccept,
                                  std::function<void(const std::string&)> onError)
    {
        std::lock_guard<std::mutex> lock(stateMutex);

        if (listenFd.load() >= 0)
            throw std::runtime_error("server already listening");

        int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s < 0)
            throw std::runtime_error(posixError("server socket() failed"));

        int yes = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(static_cast<uint16_t>(port));
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        {
            int savedErrno = errno;
            ::close(s);
            throw std::runtime_error(
                "connection:bind failed on port " + std::to_string(port) +
                " (" + std::strerror(savedErrno) + ")");
        }

        if (listen(s, SOMAXCONN) < 0)
        {
            int savedErrno = errno;
            ::close(s);
            throw std::runtime_error(
                std::string("listen failed: ") + std::strerror(savedErrno));
        }

        listenFd.store(s);
        stopping = false;

        acceptThread = std::thread([this, onAccept, onError]()
        {
            int ls = listenFd.load();
            while (!stopping.load())
            {
                sockaddr_in clientAddr{};
                socklen_t   addrLen = sizeof(clientAddr);
                int client = accept(ls,
                                    reinterpret_cast<sockaddr*>(&clientAddr),
                                    &addrLen);
                if (client < 0)
                {
                    if (stopping.load()) break;
                    if (onError)
                        onError(posixError("accept failed"));
                    continue;
                }
                if (onAccept)
                    onAccept(static_cast<uintptr_t>(client));
            }
        });
    }

    void PosixSocketServer::stop()
    {
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            int fd = listenFd.load();
            if (fd < 0 && !acceptThread.joinable())
                return;
            stopping = true;
            if (fd >= 0)
            {
                // Close the listen FD to unblock accept().
                ::close(fd);
                listenFd.store(-1);
            }
        }
        if (acceptThread.joinable())
            acceptThread.join();
    }
}

#endif // !_WIN32
