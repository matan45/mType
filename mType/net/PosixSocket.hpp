#pragma once
#include "Socket.hpp"
#include <cstdint>
#include "ISocketServer.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <memory>

namespace net
{
    // POSIX BSD-socket-backed client socket.
    class PosixSocket : public ISocket
    {
    public:
        PosixSocket();
        explicit PosixSocket(int acceptedFd);  // wrap an FD returned by accept()
        ~PosixSocket() override;

        void connect(const std::string& host, int port) override;
        int send(const std::string& data) override;
        std::string recv(int maxBytes) override;
        std::vector<uint8_t> recvBytes(int maxBytes) override;
        void close() override;
        bool isConnected() const override;
        void setTimeout(int ms) override;

    private:
        int fd;         // POSIX file descriptor (-1 = invalid)
        bool connected;
        int timeoutMs;

        void applyTimeout();
    };

    // POSIX listening server with a worker accept thread.
    class PosixSocketServer : public ISocketServer
    {
    public:
        PosixSocketServer();
        ~PosixSocketServer() override;

        void start(int port,
                   std::function<void(uintptr_t)> onAccept,
                   std::function<void(const std::string&)> onError) override;

        void stop() override;

    private:
        std::atomic<int> listenFd;
        std::thread acceptThread;
        std::atomic<bool> stopping;
        std::mutex stateMutex;
    };
}
