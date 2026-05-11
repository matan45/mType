#ifndef _WIN32

#include "PluginLoader.hpp"

#include <dlfcn.h>
#include <string>

namespace plugin
{
    void* PluginLoader::osLoad(const std::string& path, std::string& outErr)
    {
        /* Clear any prior dlerror state. */
        dlerror();
        void* h = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!h)
        {
            const char* err = dlerror();
            std::string msg = err ? err : "dlopen returned NULL";
            /* Linux's "wrong ELF class" / macOS's "incompatible architecture"
             * mean an x86-vs-x64 mismatch; surface a clearer prefix. */
            if (msg.find("wrong ELF class") != std::string::npos
                || msg.find("incompatible architecture") != std::string::npos
                || msg.find("not a mach-o") != std::string::npos)
            {
                outErr = "plugin architecture mismatch — " + msg;
            }
            else
            {
                outErr = std::move(msg);
            }
            return nullptr;
        }
        return h;
    }

    void* PluginLoader::osSym(void* handle, const char* symbol)
    {
        if (!handle || !symbol) return nullptr;
        dlerror();
        void* p = dlsym(handle, symbol);
        return p;
    }

    void PluginLoader::osClose(void* handle)
    {
        if (handle) dlclose(handle);
    }
}

#endif  /* !_WIN32 */
