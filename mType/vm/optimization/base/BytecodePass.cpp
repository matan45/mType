#include "BytecodePass.hpp"
#include "../BytecodeOptimizationResult.hpp"

namespace vm::optimization::base
{
    BytecodePass::BytecodePass(std::string name, PassType type)
        : passName(std::move(name))
        , passType(type)
    {
    }

    void BytecodePass::reportMetrics(BytecodeOptimizationResult& result) const
    {
        result.addPassMetrics({passName, transformationCount, executionTime});
    }

    void BytecodePass::reset()
    {
        transformationCount = 0;
        executionTime = std::chrono::milliseconds{0};
    }
}
