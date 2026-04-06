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

        // Console report sections
        static void printConsoleHeader(const ProfilerContext& context, double totalTimeMs);
        static void printConsoleFunctionTable(const ProfilerContext& context);
        static void printConsoleCallGraph(const ProfilerContext& context);
        static void printConsoleGCImpact(double totalTimeMs);
        static void printConsoleOpcodeCounts(const ProfilerContext& context);

        // JSON report sections
        static void emitJsonFunctions(std::ostringstream& json, const ProfilerContext& context);
        static void emitJsonCallGraph(std::ostringstream& json, const ProfilerContext& context);
        static void emitJsonGcStats(std::ostringstream& json);
        static void emitJsonOpcodes(std::ostringstream& json, const ProfilerContext& context);

        static std::string escapeJsonString(const std::string& str);
    };
}
