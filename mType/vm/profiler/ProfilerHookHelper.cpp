#include "ProfilerHookHelper.hpp"

namespace vm::profiler
{
    void ProfilerHookHelper::onFunctionEntry(const std::string& functionName)
    {
        if (!isProfilingEnabled())
        {
            return;
        }
        ProfilerContext::getInstance().recordFunctionEntry(functionName);
    }

    void ProfilerHookHelper::onFunctionExit(const std::string& functionName)
    {
        if (!isProfilingEnabled())
        {
            return;
        }
        ProfilerContext::getInstance().recordFunctionExit(functionName);
    }

    void ProfilerHookHelper::onOpcodeExecuted(uint8_t opcode)
    {
        if (!isProfilingEnabled())
        {
            return;
        }
        ProfilerContext::getInstance().recordOpcodeExecuted(opcode);
    }
}
