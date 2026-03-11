#include "ProfilerContext.hpp"

namespace vm::profiler
{
    std::unique_ptr<ProfilerContext> ProfilerContext::instance = nullptr;
    std::mutex ProfilerContext::instanceMutex;

    ProfilerContext::ProfilerContext()
        : mode(ProfilerMode::DISABLED)
        , outputFormat(ProfilerOutputFormat::CONSOLE)
    {
    }

    ProfilerContext& ProfilerContext::getInstance()
    {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (!instance)
        {
            instance = std::unique_ptr<ProfilerContext>(new ProfilerContext());
        }
        return *instance;
    }

    void ProfilerContext::initialize(ProfilerMode mode, ProfilerOutputFormat format)
    {
        auto& ctx = getInstance();
        ctx.mode = mode;
        ctx.outputFormat = format;
        ctx.functionProfiles.clear();
        ctx.callGraphEdges.clear();
        ctx.timingStack.clear();
        ctx.opcodeProfile = OpcodeProfile{};
        ctx.totalProfilingTimeNs = 0;
        ctx.profilingStartTime = std::chrono::high_resolution_clock::now();
    }

    void ProfilerContext::shutdown()
    {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (instance)
        {
            instance.reset();
        }
    }

    void ProfilerContext::recordFunctionEntry(const std::string& functionName)
    {
        // Record call graph edge (full mode only)
        if (mode == ProfilerMode::FULL && !timingStack.empty())
        {
            const std::string& caller = timingStack.back().functionName;
            std::string edgeKey = caller + "->" + functionName;
            auto& edge = callGraphEdges[edgeKey];
            if (edge.caller.empty())
            {
                edge.caller = caller;
                edge.callee = functionName;
            }
            edge.callCount++;
        }

        // Push timing entry
        TimingStackEntry entry;
        entry.functionName = functionName;
        entry.entryTime = std::chrono::high_resolution_clock::now();
        entry.childTimeNs = 0;
        timingStack.push_back(std::move(entry));

        // Increment call count
        auto& profile = functionProfiles[functionName];
        if (profile.functionName.empty())
        {
            profile.functionName = functionName;
        }
        profile.callCount++;
    }

    void ProfilerContext::recordFunctionExit(const std::string& functionName)
    {
        if (timingStack.empty())
        {
            return;
        }

        // Find the matching entry (should be the back)
        TimingStackEntry entry = timingStack.back();
        timingStack.pop_back();

        auto now = std::chrono::high_resolution_clock::now();
        uint64_t elapsed = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now - entry.entryTime).count());

        uint64_t selfTime = elapsed > entry.childTimeNs ? elapsed - entry.childTimeNs : 0;

        // Update function profile
        auto& profile = functionProfiles[entry.functionName];
        profile.totalTimeNs += elapsed;
        profile.selfTimeNs += selfTime;

        // Add elapsed time to parent's childTimeNs
        if (!timingStack.empty())
        {
            timingStack.back().childTimeNs += elapsed;
        }

        // Update call graph edge timing (full mode only)
        if (mode == ProfilerMode::FULL && !timingStack.empty())
        {
            const std::string& caller = timingStack.back().functionName;
            std::string edgeKey = caller + "->" + entry.functionName;
            auto it = callGraphEdges.find(edgeKey);
            if (it != callGraphEdges.end())
            {
                it->second.totalTimeNs += elapsed;
            }
        }
    }

    void ProfilerContext::recordOpcodeExecuted(uint8_t opcode)
    {
        opcodeProfile.counts[opcode]++;
    }

    void ProfilerContext::unwindToDepth(size_t targetDepth)
    {
        while (timingStack.size() > targetDepth)
        {
            recordFunctionExit(timingStack.back().functionName);
        }
    }

    void ProfilerContext::finalize()
    {
        // Close any remaining timing stack entries
        while (!timingStack.empty())
        {
            recordFunctionExit(timingStack.back().functionName);
        }

        auto now = std::chrono::high_resolution_clock::now();
        totalProfilingTimeNs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(now - profilingStartTime).count());
    }
}
