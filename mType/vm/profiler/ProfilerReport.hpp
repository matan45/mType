#pragma once

#include "ProfilerContext.hpp"
#include <string>

namespace vm::profiler
{
    class ProfilerReport
    {
    public:
        static void generate(const ProfilerContext& context);

    private:
        static void generateConsoleReport(const ProfilerContext& context);
        static void generateJsonReport(const ProfilerContext& context);
        static std::string escapeJsonString(const std::string& str);
    };
}
