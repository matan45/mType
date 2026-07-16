#include "JitProfiler.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace vm::jit
{
    JitProfiler::JitProfiler(uint32_t hotThreshold)
        : hotThreshold(hotThreshold)
    {
    }

    uint32_t JitProfiler::thresholdForBodySize(size_t bytecodeBodySize) const noexcept
    {
        // Bodies up to 32 bytecodes retain the old behaviour. Larger bodies
        // perform more interpreted work per invocation, so discount at most
        // 25% from the base threshold. The independent code-cache byte budget
        // is the appropriate guard against native-code bloat; this policy only
        // adjusts when compilation amortises over interpreted work.
        if (bytecodeBodySize == 0 || bytecodeBodySize <= 32 || hotThreshold == 0)
        {
            return hotThreshold;
        }

        const uint64_t base = hotThreshold;
        const uint64_t extraBytecodes = bytecodeBodySize - 32;
        const uint64_t maxDiscount = base / 4;
        const uint64_t discount = extraBytecodes >= 256
            ? maxDiscount
            : std::min<uint64_t>((extraBytecodes * base) / 1024,
                                 maxDiscount);
        return static_cast<uint32_t>(base - discount);
    }

    bool JitProfiler::recordEntry(bytecode::ProgramId programId,
                                  std::string_view functionName,
                                  size_t bytecodeBodySize)
    {
        const FunctionLookupId lookup{programId, functionName};
        auto it = invocationProfiles.find(lookup);
        if (it == invocationProfiles.end())
        {
            FunctionId function{programId, std::string(functionName)};
            FunctionProfile initial{};
            initial.effectiveThreshold = thresholdForBodySize(bytecodeBodySize);
            it = invocationProfiles.emplace(std::move(function), initial).first;
        }

        auto& profile = it->second;
        if (profile.invocationCount != std::numeric_limits<uint32_t>::max())
        {
            ++profile.invocationCount;
        }

        if (profile.invocationCount == profile.effectiveThreshold)
        {
            hotFunctions.push_back(it->first);
            return true;
        }

        return false;
    }

    bool JitProfiler::isHot(const FunctionId& function) const
    {
        auto it = invocationProfiles.find(function);
        return it != invocationProfiles.end()
            && it->second.invocationCount >= it->second.effectiveThreshold;
    }

    uint32_t JitProfiler::getInvocationCount(const FunctionId& function) const
    {
        auto it = invocationProfiles.find(function);
        return it != invocationProfiles.end() ? it->second.invocationCount : 0;
    }

    uint32_t JitProfiler::getEffectiveThreshold(
        const FunctionId& function,
        size_t bytecodeBodySize) const
    {
        auto it = invocationProfiles.find(function);
        return it != invocationProfiles.end()
            ? it->second.effectiveThreshold
            : thresholdForBodySize(bytecodeBodySize);
    }

    const std::vector<FunctionId>& JitProfiler::getHotFunctions() const
    {
        return hotFunctions;
    }

    void JitProfiler::reset()
    {
        invocationProfiles.clear();
        hotFunctions.clear();
    }
}
