#include "JitProfiler.hpp"

namespace vm::jit
{
    JitProfiler::JitProfiler(uint32_t hotThreshold)
        : hotThreshold(hotThreshold)
    {
    }

    bool JitProfiler::recordEntry(const std::string& functionName)
    {
        auto& count = invocationCounts[functionName];
        ++count;

        if (count == hotThreshold)
        {
            hotFunctions.push_back(functionName);
            return true;
        }

        return false;
    }

    bool JitProfiler::isHot(const std::string& functionName) const
    {
        auto it = invocationCounts.find(functionName);
        return it != invocationCounts.end() && it->second >= hotThreshold;
    }

    uint32_t JitProfiler::getInvocationCount(const std::string& functionName) const
    {
        auto it = invocationCounts.find(functionName);
        return it != invocationCounts.end() ? it->second : 0;
    }

    const std::vector<std::string>& JitProfiler::getHotFunctions() const
    {
        return hotFunctions;
    }

    void JitProfiler::reset()
    {
        invocationCounts.clear();
        hotFunctions.clear();
    }
}
