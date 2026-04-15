#pragma once
#include "Socket.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <memory>

namespace net
{
    // WinSock2-backed client socket.
    class WinSocket : public ISocket
    {
    public:
        WinSocket();
        explicit WinSocket(uintptr_t acceptedFd);  // wrap an FD returned by accept()
        ~WinSocket() override;

        void connect(const std::string& host, int port) override;
        int send(const std::string& data) override;
        std::string recv(int maxBytes) override;
        std::vector<uint8_t> recvBytes(int maxBytes) override;
        void close() override;
        bool isConnected() const override;
        void setTimeout(int ms) override;

    private:
        uintptr_t fd;        // SOCKET handle (cast to uintptr_t to avoid winsock include in header)
        bool connected;
        int timeoutMs;
    };

    // WinSock2-backed listening server with a worker accept thread.
    class WinSocketServer
    {
    public:
        WinSocketServer();
        ~WinSocketServer();

        // Bind + listen on the given port. Spawns the accept thread.
        // onAccept is invoked on the worker thread for each accepted client FD.
        // onError is invoked on the worker thread for accept-loop errors.
        void start(int port,
                   std::function<void(uintptr_t)> onAccept,
                   std::function<void(const std::string&)> onError);

        void stop();

    private:
        uintptr_t listenFd;
        std::thread acceptThread;
        std::atomic<bool> stopping;
        std::mutex stateMutex;
    };
}
