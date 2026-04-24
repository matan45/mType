#include "ProfilerReport.hpp"
#include "../bytecode/OpCode.hpp"
#include "../../gc/GC.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cstdio>

namespace vm::profiler
{
    void ProfilerReport::generate(const ProfilerContext& context)
    {
        if (context.getOutputFormat() == ProfilerOutputFormat::JSON)
        {
            generateJsonReport(context);
        }
        else
        {
            generateConsoleReport(context);
        }
    }

    void ProfilerReport::generateConsoleReport(const ProfilerContext& context)
    {
        double totalTimeMs = static_cast<double>(context.getTotalProfilingTimeNs()) / 1e6;

        printConsoleHeader(context, totalTimeMs);
        printConsoleFunctionTable(context);

        if (context.getMode() == ProfilerMode::FULL)
        {
            printConsoleCallGraph(context);
        }

        printConsoleGCImpact(totalTimeMs);

        if (context.getMode() == ProfilerMode::FULL)
        {
            printConsoleOpcodeCounts(context);
            printConsoleOpcodePairs(context);
        }

        std::cout << "=============================\n";
    }

    void ProfilerReport::printConsoleHeader(const ProfilerContext& context, double totalTimeMs)
    {
        std::string modeStr = context.getMode() == ProfilerMode::FULL ? "full" : "light";
        std::cout << "\n=== mType Profiler Report ===\n";
        std::cout << "Mode: " << modeStr << " | Total time: "
                  << std::fixed << std::setprecision(2) << totalTimeMs << " ms\n";
    }

    void ProfilerReport::printConsoleFunctionTable(const ProfilerContext& context)
    {
        const auto& profiles = context.getFunctionProfiles();
        std::vector<const FunctionProfile*> sorted;
        sorted.reserve(profiles.size());
        for (const auto& [name, profile] : profiles)
        {
            sorted.push_back(&profile);
        }
        std::sort(sorted.begin(), sorted.end(), [](const FunctionProfile* a, const FunctionProfile* b)
        {
            return a->totalTimeNs > b->totalTimeNs;
        });

        size_t limit = std::min(sorted.size(), static_cast<size_t>(10));
        if (limit == 0) return;

        std::cout << "\n--- Top " << limit << " Functions by Total Time ---\n";
        std::cout << std::setw(4) << "#"
                  << "  " << std::left << std::setw(40) << "Function"
                  << std::right << std::setw(8) << "Calls"
                  << std::setw(14) << "Total (ms)"
                  << std::setw(14) << "Self (ms)"
                  << std::setw(12) << "Avg (us)" << "\n";

        for (size_t i = 0; i < limit; ++i)
        {
            const auto* p = sorted[i];
            double totalMs = static_cast<double>(p->totalTimeNs) / 1e6;
            double selfMs = static_cast<double>(p->selfTimeNs) / 1e6;
            double avgUs = p->callCount > 0 ? static_cast<double>(p->totalTimeNs) / (p->callCount * 1000.0) : 0.0;

            std::string displayName = p->functionName;
            if (displayName.length() > 38)
            {
                displayName = displayName.substr(0, 35) + "...";
            }

            std::cout << std::setw(4) << (i + 1)
                      << "  " << std::left << std::setw(40) << displayName
                      << std::right << std::setw(8) << p->callCount
                      << std::setw(14) << std::fixed << std::setprecision(2) << totalMs
                      << std::setw(14) << selfMs
                      << std::setw(12) << avgUs << "\n";
        }
    }

    void ProfilerReport::printConsoleCallGraph(const ProfilerContext& context)
    {
        const auto& edges = context.getCallGraphEdges();
        if (edges.empty()) return;

        std::vector<const CallGraphEdge*> sortedEdges;
        sortedEdges.reserve(edges.size());
        for (const auto& [key, edge] : edges)
        {
            sortedEdges.push_back(&edge);
        }
        std::sort(sortedEdges.begin(), sortedEdges.end(), [](const CallGraphEdge* a, const CallGraphEdge* b)
        {
            return a->totalTimeNs > b->totalTimeNs;
        });

        size_t edgeLimit = std::min(sortedEdges.size(), static_cast<size_t>(10));
        std::cout << "\n--- Call Graph (top " << edgeLimit << ") ---\n";

        for (size_t i = 0; i < edgeLimit; ++i)
        {
            const auto* e = sortedEdges[i];
            double timeMs = static_cast<double>(e->totalTimeNs) / 1e6;

            std::string caller = e->caller.length() > 25 ? e->caller.substr(0, 22) + "..." : e->caller;
            std::string callee = e->callee.length() > 25 ? e->callee.substr(0, 22) + "..." : e->callee;

            std::cout << "  " << caller << " -> " << callee
                      << "  " << e->callCount << " calls  "
                      << std::fixed << std::setprecision(2) << timeMs << " ms\n";
        }
    }

    void ProfilerReport::printConsoleGCImpact(double totalTimeMs)
    {
        const auto* gcStats = gc::GC::getStats();
        if (!gcStats || gcStats->totalCollections.load() == 0) return;

        double gcTimeMs = static_cast<double>(gcStats->totalCollectionTimeUs.load()) / 1000.0;
        double gcPercent = totalTimeMs > 0 ? (gcTimeMs / totalTimeMs) * 100.0 : 0.0;

        std::cout << "\n--- GC Impact ---\n";
        std::cout << "  Collections: " << gcStats->totalCollections.load()
                  << " | GC time: " << std::fixed << std::setprecision(2) << gcTimeMs
                  << " ms (" << std::setprecision(1) << gcPercent << "% of execution)\n";
    }

    void ProfilerReport::printConsoleOpcodeCounts(const ProfilerContext& context)
    {
        const auto& opcodeProfile = context.getOpcodeProfile();

        std::vector<std::pair<uint8_t, uint64_t>> opcodeCounts;
        for (size_t i = 0; i < 256; ++i)
        {
            if (opcodeProfile.counts[i] > 0)
            {
                opcodeCounts.emplace_back(static_cast<uint8_t>(i), opcodeProfile.counts[i]);
            }
        }

        if (opcodeCounts.empty()) return;

        std::sort(opcodeCounts.begin(), opcodeCounts.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        size_t opcodeLimit = std::min(opcodeCounts.size(), static_cast<size_t>(10));
        std::cout << "\n--- Opcode Execution Counts (top " << opcodeLimit << ") ---\n";
        std::cout << "  ";

        for (size_t i = 0; i < opcodeLimit; ++i)
        {
            if (i > 0) std::cout << " | ";
            auto opcode = static_cast<bytecode::OpCode>(opcodeCounts[i].first);
            std::cout << bytecode::getOpCodeName(opcode) << ": " << opcodeCounts[i].second;
        }
        std::cout << "\n";
    }

    void ProfilerReport::printConsoleOpcodePairs(const ProfilerContext& context)
    {
        const auto& opcodeProfile = context.getOpcodeProfile();

        // pairCounts is allocated lazily — skip the walk if FULL mode never
        // fired recordOpcodeExecuted with a prior opcode.
        if (!opcodeProfile.pairCounts) return;

        std::vector<std::pair<uint16_t, uint64_t>> pairs;
        for (size_t i = 0; i < OpcodeProfile::pairCountsSize(); ++i)
        {
            uint64_t count = (*opcodeProfile.pairCounts)[i];
            if (count > 0)
            {
                pairs.emplace_back(static_cast<uint16_t>(i), count);
            }
        }

        if (pairs.empty()) return;

        std::sort(pairs.begin(), pairs.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        size_t limit = std::min(pairs.size(), static_cast<size_t>(30));
        std::cout << "\n--- Opcode Pair Counts (top " << limit << ") [MYT-202 fusion targets] ---\n";

        for (size_t i = 0; i < limit; ++i)
        {
            uint16_t key = pairs[i].first;
            auto prev = static_cast<bytecode::OpCode>(static_cast<uint8_t>(key >> 8));
            auto curr = static_cast<bytecode::OpCode>(static_cast<uint8_t>(key & 0xFF));
            std::cout << "  " << std::setw(4) << (i + 1) << "  "
                      << bytecode::getOpCodeName(prev) << " + "
                      << bytecode::getOpCodeName(curr) << " : "
                      << pairs[i].second << "\n";
        }
    }

    void ProfilerReport::generateJsonReport(const ProfilerContext& context)
    {
        std::ostringstream json;
        json << "{\n";

        double totalTimeMs = static_cast<double>(context.getTotalProfilingTimeNs()) / 1e6;
        std::string modeStr = context.getMode() == ProfilerMode::FULL ? "full" : "light";
        json << "  \"mode\": \"" << modeStr << "\",\n";
        json << "  \"totalTimeMs\": " << std::fixed << std::setprecision(4) << totalTimeMs << ",\n";

        emitJsonFunctions(json, context);
        emitJsonCallGraph(json, context);
        emitJsonGcStats(json);
        emitJsonOpcodes(json, context);
        emitJsonOpcodePairs(json, context);

        json << "}\n";
        std::cout << json.str();
    }

    void ProfilerReport::emitJsonFunctions(std::ostringstream& json, const ProfilerContext& context)
    {
        json << "  \"functions\": [\n";
        const auto& profiles = context.getFunctionProfiles();
        size_t funcIdx = 0;
        for (const auto& [name, profile] : profiles)
        {
            if (funcIdx > 0) json << ",\n";
            json << "    {";
            json << "\"name\": \"" << escapeJsonString(profile.functionName) << "\", ";
            json << "\"calls\": " << profile.callCount << ", ";
            json << "\"totalTimeNs\": " << profile.totalTimeNs << ", ";
            json << "\"selfTimeNs\": " << profile.selfTimeNs;
            json << "}";
            funcIdx++;
        }
        json << "\n  ],\n";
    }

    void ProfilerReport::emitJsonCallGraph(std::ostringstream& json, const ProfilerContext& context)
    {
        json << "  \"callGraph\": [\n";
        const auto& edges = context.getCallGraphEdges();
        size_t edgeIdx = 0;
        for (const auto& [key, edge] : edges)
        {
            if (edgeIdx > 0) json << ",\n";
            json << "    {";
            json << "\"caller\": \"" << escapeJsonString(edge.caller) << "\", ";
            json << "\"callee\": \"" << escapeJsonString(edge.callee) << "\", ";
            json << "\"calls\": " << edge.callCount << ", ";
            json << "\"totalTimeNs\": " << edge.totalTimeNs;
            json << "}";
            edgeIdx++;
        }
        json << "\n  ],\n";
    }

    void ProfilerReport::emitJsonGcStats(std::ostringstream& json)
    {
        const auto* gcStats = gc::GC::getStats();
        json << "  \"gcStats\": {";
        if (gcStats)
        {
            json << "\"collections\": " << gcStats->totalCollections.load() << ", ";
            json << "\"totalTimeUs\": " << gcStats->totalCollectionTimeUs.load() << ", ";
            json << "\"objectsCollected\": " << gcStats->objectsCollected.load();
        }
        json << "},\n";
    }

    void ProfilerReport::emitJsonOpcodes(std::ostringstream& json, const ProfilerContext& context)
    {
        json << "  \"opcodes\": {";
        if (context.getMode() == ProfilerMode::FULL)
        {
            const auto& opcodeProfile = context.getOpcodeProfile();
            bool first = true;
            for (size_t i = 0; i < 256; ++i)
            {
                if (opcodeProfile.counts[i] > 0)
                {
                    if (!first) json << ", ";
                    auto opcode = static_cast<bytecode::OpCode>(static_cast<uint8_t>(i));
                    json << "\"" << bytecode::getOpCodeName(opcode) << "\": " << opcodeProfile.counts[i];
                    first = false;
                }
            }
        }
        json << "},\n";
    }

    void ProfilerReport::emitJsonOpcodePairs(std::ostringstream& json, const ProfilerContext& context)
    {
        json << "  \"opcodePairs\": [";
        if (context.getMode() == ProfilerMode::FULL)
        {
            const auto& opcodeProfile = context.getOpcodeProfile();

            std::vector<std::pair<uint16_t, uint64_t>> pairs;
            if (opcodeProfile.pairCounts)
            {
                for (size_t i = 0; i < OpcodeProfile::pairCountsSize(); ++i)
                {
                    uint64_t count = (*opcodeProfile.pairCounts)[i];
                    if (count > 0)
                    {
                        pairs.emplace_back(static_cast<uint16_t>(i), count);
                    }
                }
            }
            std::sort(pairs.begin(), pairs.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });

            size_t limit = std::min(pairs.size(), static_cast<size_t>(100));
            for (size_t i = 0; i < limit; ++i)
            {
                if (i > 0) json << ", ";
                uint16_t key = pairs[i].first;
                auto prev = static_cast<bytecode::OpCode>(static_cast<uint8_t>(key >> 8));
                auto curr = static_cast<bytecode::OpCode>(static_cast<uint8_t>(key & 0xFF));
                json << "{\"prev\": \"" << bytecode::getOpCodeName(prev)
                     << "\", \"curr\": \"" << bytecode::getOpCodeName(curr)
                     << "\", \"count\": " << pairs[i].second << "}";
            }
        }
        json << "]\n";
    }

    std::string ProfilerReport::escapeJsonString(const std::string& str)
    {
        std::string result;
        result.reserve(str.size());
        for (unsigned char c : str)
        {
            switch (c)
            {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (c < 0x20)
                    {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                        result += buf;
                    }
                    else
                    {
                        result += static_cast<char>(c);
                    }
                    break;
            }
        }
        return result;
    }
}
