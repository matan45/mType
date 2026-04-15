#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace net
{
    // Abstract socket interface so a future POSIX impl can drop in beside WinSocket.
    class ISocket
    {
    public:
        virtual ~ISocket() = default;

        virtual void connect(const std::string& host, int port) = 0;
        virtual int send(const std::string& data) = 0;
        virtual std::string recv(int maxBytes) = 0;
        virtual std::vector<uint8_t> recvBytes(int maxBytes) = 0;
        virtual void close() = 0;
        virtual bool isConnected() const = 0;
        virtual void setTimeout(int ms) = 0;
    };
}
