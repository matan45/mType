#include "PluginLoader.hpp"

#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace plugin
{
    namespace
    {
        std::wstring utf8ToWide(const std::string& utf8)
        {
            if (utf8.empty()) return {};
            int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                        static_cast<int>(utf8.size()), nullptr, 0);
            std::wstring out(static_cast<size_t>(n), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                static_cast<int>(utf8.size()), out.data(), n);
            return out;
        }

        std::string formatLastError(DWORD err)
        {
            LPSTR buf = nullptr;
            DWORD n = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPSTR>(&buf), 0, nullptr);
            std::string msg = (n > 0 && buf) ? std::string(buf, n) : "unknown Windows error";
            if (buf) LocalFree(buf);
            /* Strip trailing CR/LF FormatMessage tacks on. */
            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r'))
            {
                msg.pop_back();
            }
            return msg;
        }
    }

    void* PluginLoader::osLoad(const std::string& path, std::string& outErr)
    {
        std::wstring wpath = utf8ToWide(path);
        HMODULE h = LoadLibraryW(wpath.c_str());
        if (!h)
        {
            DWORD err = GetLastError();
            if (err == ERROR_BAD_EXE_FORMAT)
            {
                outErr = "plugin architecture mismatch (x86 vs x64) — code 193";
            }
            else
            {
                outErr = formatLastError(err);
            }
            return nullptr;
        }
        return static_cast<void*>(h);
    }

    void* PluginLoader::osSym(void* handle, const char* symbol)
    {
        if (!handle || !symbol) return nullptr;
        FARPROC p = GetProcAddress(static_cast<HMODULE>(handle), symbol);
        return reinterpret_cast<void*>(p);
    }

    void PluginLoader::osClose(void* handle)
    {
        if (handle) FreeLibrary(static_cast<HMODULE>(handle));
    }
}
