#pragma once

#include "ProfilerContext.hpp"
#include <string>
#include <cstdint>

namespace vm::profiler
{
    class ProfilerHookHelper
    {
    public:
        static inline bool isProfilingEnabled()
        {
            return ProfilerContext::isEnabledFast();
        }

        static void onFunctionEntry(const std::string& functionName);
        static void onFunctionExit(const std::string& functionName);
        static void onOpcodeExecuted(uint8_t opcode);
    };
}
