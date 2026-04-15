#pragma once

namespace net
{
    // RAII singleton wrapping WSAStartup / WSACleanup.
    // First call to ensure() initializes Winsock; cleanup happens at process exit.
    class WinSockInit
    {
    public:
        static void ensure();

    private:
        WinSockInit();
        ~WinSockInit();
        WinSockInit(const WinSockInit&) = delete;
        WinSockInit& operator=(const WinSockInit&) = delete;

        bool initialized;
    };
}
