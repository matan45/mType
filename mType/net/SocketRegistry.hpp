#pragma once
#include "Socket.hpp"
#include "ISocketServer.hpp"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <vector>

namespace net
{
    // Thread-safe registry of socket / server resources keyed by integer handles
    // returned to mType code. Lookups throw on invalid or already-closed handles.
    class SocketRegistry
    {
    public:
        static SocketRegistry& instance();

        // Client sockets ----------------------------------------------------
        int registerSocket(std::shared_ptr<ISocket> socket);
        std::shared_ptr<ISocket> getSocket(int handle);  // throws if missing
        void closeSocket(int handle);                    // idempotent

        // Server sockets ----------------------------------------------------
        int registerServer(std::shared_ptr<ISocketServer> server);
        std::shared_ptr<ISocketServer> getServer(int handle);
        void closeServer(int handle);

        // Forced cleanup at process / env teardown.
        void cleanup();

    private:
        SocketRegistry() = default;

        std::mutex mutex;
        std::unordered_map<int, std::shared_ptr<ISocket>> sockets;
        std::unordered_map<int, std::shared_ptr<ISocketServer>> servers;
        std::atomic<int> nextSocketHandle{1};
        std::atomic<int> nextServerHandle{1};
    };
}
