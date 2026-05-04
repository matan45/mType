#include "SocketRegistry.hpp"
#include <stdexcept>

namespace net
{
    SocketRegistry& SocketRegistry::instance()
    {
        static SocketRegistry inst;
        return inst;
    }

    int SocketRegistry::registerSocket(std::shared_ptr<ISocket> socket)
    {
        std::lock_guard<std::mutex> lock(mutex);
        int handle = nextSocketHandle.fetch_add(1);
        if (handle <= 0)
        {
            throw std::runtime_error("connection:socket handle counter exhausted");
        }
        sockets[handle] = std::move(socket);
        return handle;
    }

    std::shared_ptr<ISocket> SocketRegistry::getSocket(int handle)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = sockets.find(handle);
        if (it == sockets.end() || !it->second)
        {
            throw std::runtime_error("connection:invalid or closed socket handle " + std::to_string(handle));
        }
        return it->second;
    }

    void SocketRegistry::closeSocket(int handle)
    {
        std::shared_ptr<ISocket> sock;
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto it = sockets.find(handle);
            if (it == sockets.end()) return;
            sock = it->second;
            sockets.erase(it);
        }
        if (sock)
        {
            try { sock->close(); } catch (...) {}
        }
    }

    int SocketRegistry::registerServer(std::shared_ptr<ISocketServer> server)
    {
        std::lock_guard<std::mutex> lock(mutex);
        int handle = nextServerHandle.fetch_add(1);
        if (handle <= 0)
        {
            throw std::runtime_error("connection:server handle counter exhausted");
        }
        servers[handle] = std::move(server);
        return handle;
    }

    std::shared_ptr<ISocketServer> SocketRegistry::getServer(int handle)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = servers.find(handle);
        if (it == servers.end() || !it->second)
        {
            throw std::runtime_error("connection:invalid or closed server handle " + std::to_string(handle));
        }
        return it->second;
    }

    void SocketRegistry::closeServer(int handle)
    {
        std::shared_ptr<ISocketServer> srv;
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto it = servers.find(handle);
            if (it == servers.end()) return;
            srv = it->second;
            servers.erase(it);
        }
        if (srv)
        {
            try { srv->stop(); } catch (...) {}
        }
    }

    void SocketRegistry::cleanup()
    {
        std::unordered_map<int, std::shared_ptr<ISocket>> socksCopy;
        std::unordered_map<int, std::shared_ptr<ISocketServer>> serversCopy;
        {
            std::lock_guard<std::mutex> lock(mutex);
            socksCopy.swap(sockets);
            serversCopy.swap(servers);
        }
        for (auto& kv : serversCopy)
        {
            if (kv.second) try { kv.second->stop(); } catch (...) {}
        }
        for (auto& kv : socksCopy)
        {
            if (kv.second) try { kv.second->close(); } catch (...) {}
        }
    }
}
