#include "ProfilerContext.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace vm::profiler
{
    std::unique_ptr<ProfilerContext> ProfilerContext::instance = nullptr;
    bool ProfilerContext::enabledFlag = false;

    ProfilerContext::ProfilerContext()
        : mode(ProfilerMode::DISABLED)
        , outputFormat(ProfilerOutputFormat::CONSOLE)
    {
    }

    ProfilerContext& ProfilerContext::getInstance()
    {
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
        ctx.lastOpcode = 0;
        ctx.hasLast = false;
        ctx.totalProfilingTimeNs = 0;
        ctx.profilingStartTime = std::chrono::high_resolution_clock::now();
        enabledFlag = (mode != ProfilerMode::DISABLED);
    }

    void ProfilerContext::shutdown()
    {
        enabledFlag = false;
        if (instance)
        {
            instance.reset();
        }
    }

    void ProfilerContext::recordFunctionEntry(const std::string& functionName)
    {
        // MYT-202: pair-tracking resets at frame boundaries.
        hasLast = false;

        if (mode == ProfilerMode::FULL && !timingStack.empty())
        {
            recordCallGraphEntry(timingStack.back().functionName, functionName);
        }

        TimingStackEntry entry;
        entry.functionName = functionName;
        entry.entryTime = std::chrono::high_resolution_clock::now();
        entry.childTimeNs = 0;
        timingStack.push_back(std::move(entry));

        auto& profile = functionProfiles[functionName];
        if (profile.functionName.empty())
        {
            profile.functionName = functionName;
        }
        profile.callCount++;
    }

    void ProfilerContext::recordFunctionExit(const std::string& functionName)
    {
        // MYT-202: pair-tracking resets at frame boundaries.
        hasLast = false;

        if (timingStack.empty())
        {
            return;
        }

        // Validate that exit matches the expected entry
        TimingStackEntry entry = timingStack.back();
        timingStack.pop_back();

        if (entry.functionName != functionName)
        {
            std::cerr << "[Profiler] Warning: mismatched function exit. Expected '"
                      << entry.functionName << "', got '" << functionName << "'\n";
        }

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

        if (mode == ProfilerMode::FULL && !timingStack.empty())
        {
            recordCallGraphExit(timingStack.back().functionName, entry.functionName, elapsed);
        }
    }

    void ProfilerContext::recordCallGraphEntry(const std::string& caller, const std::string& callee)
    {
        std::string edgeKey = caller + "->" + callee;
        auto& edge = callGraphEdges[edgeKey];
        if (edge.caller.empty())
        {
            edge.caller = caller;
            edge.callee = callee;
        }
        edge.callCount++;
    }

    void ProfilerContext::recordCallGraphExit(const std::string& caller, const std::string& callee, uint64_t elapsedNs)
    {
        std::string edgeKey = caller + "->" + callee;
        auto it = callGraphEdges.find(edgeKey);
        if (it != callGraphEdges.end())
        {
            it->second.totalTimeNs += elapsedNs;
        }
    }

    void ProfilerContext::recordOpcodeExecuted(uint8_t opcode)
    {
        opcodeProfile.counts[opcode]++;
        // MYT-202: track adjacent-pair frequency so the peephole pass can be
        // retargeted from data rather than guesswork. 512 KiB table is
        // allocated lazily on first pair observation to keep non-FULL runs
        // from reserving it.
        if (hasLast)
        {
            if (!opcodeProfile.pairCounts)
                opcodeProfile.pairCounts = std::make_unique<std::array<uint64_t, 65536>>();
            (*opcodeProfile.pairCounts)[(static_cast<uint16_t>(lastOpcode) << 8) | opcode]++;
        }
        lastOpcode = opcode;
        hasLast = true;
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
