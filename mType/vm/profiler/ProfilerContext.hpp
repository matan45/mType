#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include "ProfilerMode.hpp"
#include "ProfilerData.hpp"

namespace vm::profiler
{
    class ProfilerContext
    {
    private:
        static std::unique_ptr<ProfilerContext> instance;
        static std::mutex instanceMutex;

        // Fast-path flag: avoids mutex/getInstance() on every profiling check
        static bool enabledFlag;

        ProfilerMode mode;
        ProfilerOutputFormat outputFormat;

        // Function profiles keyed by function name
        std::unordered_map<std::string, FunctionProfile> functionProfiles;

        // Call graph edges keyed by "caller->callee"
        std::unordered_map<std::string, CallGraphEdge> callGraphEdges;

        // Timing stack for tracking entry/exit
        std::vector<TimingStackEntry> timingStack;

        // Opcode execution counts (full mode only)
        OpcodeProfile opcodeProfile;

        // Total profiling time
        std::chrono::high_resolution_clock::time_point profilingStartTime;
        uint64_t totalProfilingTimeNs = 0;

        ProfilerContext();

    public:
        static ProfilerContext& getInstance();
        static void initialize(ProfilerMode mode, ProfilerOutputFormat format = ProfilerOutputFormat::CONSOLE);
        static void shutdown();

        ProfilerContext(const ProfilerContext&) = delete;
        ProfilerContext& operator=(const ProfilerContext&) = delete;

        // Fast-path check — no mutex, no getInstance()
        static bool isEnabledFast() { return enabledFlag; }

        // Mode queries
        ProfilerMode getMode() const { return mode; }
        ProfilerOutputFormat getOutputFormat() const { return outputFormat; }
        bool isEnabled() const { return mode != ProfilerMode::DISABLED; }
        bool isFullMode() const { return mode == ProfilerMode::FULL; }

        // Recording
        void recordFunctionEntry(const std::string& functionName);
        void recordFunctionExit(const std::string& functionName);
        void recordOpcodeExecuted(uint8_t opcode);

        // Unwinding support (exception paths)
        void unwindToDepth(size_t targetDepth);
        size_t getTimingStackDepth() const { return timingStack.size(); }

        // Data access for report generation
        const std::unordered_map<std::string, FunctionProfile>& getFunctionProfiles() const { return functionProfiles; }
        const std::unordered_map<std::string, CallGraphEdge>& getCallGraphEdges() const { return callGraphEdges; }
        const OpcodeProfile& getOpcodeProfile() const { return opcodeProfile; }
        uint64_t getTotalProfilingTimeNs() const { return totalProfilingTimeNs; }

        // Finalize timing
        void finalize();

    private:
        // Call graph helpers (full mode only)
        void recordCallGraphEntry(const std::string& caller, const std::string& callee);
        void recordCallGraphExit(const std::string& caller, const std::string& callee, uint64_t elapsedNs);
    };
}
