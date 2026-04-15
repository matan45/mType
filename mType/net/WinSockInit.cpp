#include "WinSockInit.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>
#include <string>

#pragma comment(lib, "ws2_32.lib")

namespace net
{
    void WinSockInit::ensure()
    {
        static WinSockInit instance;
        (void)instance;
    }

    WinSockInit::WinSockInit() : initialized(false)
    {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            throw std::runtime_error("WSAStartup failed: " + std::to_string(result));
        }
        initialized = true;
    }

    WinSockInit::~WinSockInit()
    {
        if (initialized)
        {
            WSACleanup();
        }
    }
}
