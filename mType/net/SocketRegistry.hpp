#pragma once
#include "Socket.hpp"
#include "WinSocket.hpp"
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
        struct ServerRecord
        {
            std::shared_ptr<WinSocketServer> server;
            // Stored callbacks - referenced as opaque void* + holder vector to
            // keep the registry header free of value::Value / VM types.
            // Actual storage is in NetNatives so the registry stays neutral.
        };

        int registerServer(std::shared_ptr<WinSocketServer> server);
        std::shared_ptr<WinSocketServer> getServer(int handle);
        void closeServer(int handle);

        // Forced cleanup at process / env teardown.
        void cleanup();

    private:
        SocketRegistry() = default;

        std::mutex mutex;
        std::unordered_map<int, std::shared_ptr<ISocket>> sockets;
        std::unordered_map<int, std::shared_ptr<WinSocketServer>> servers;
        std::atomic<int> nextSocketHandle{1};
        std::atomic<int> nextServerHandle{1};
    };
}
