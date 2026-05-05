#pragma once
#include <functional>
#include <cstdint>
#include <string>

namespace net
{
    // Abstract server socket interface so Win and POSIX implementations
    // can be stored uniformly in SocketRegistry.
    class ISocketServer
    {
    public:
        virtual ~ISocketServer() = default;

        virtual void start(int port,
                           std::function<void(uintptr_t)> onAccept,
                           std::function<void(const std::string&)> onError) = 0;

        virtual void stop() = 0;
    };
}
