#include "BytecodeOptimizationConfig.hpp"

namespace vm::optimization
{
    BytecodeOptimizationConfig BytecodeOptimizationConfig::forRelease()
    {
        // Default-constructed config has every pass enabled — matches the
        // historic compile() behaviour where every pass fired on every
        // compile regardless of source language opt level.
        return BytecodeOptimizationConfig{};
    }

    BytecodeOptimizationConfig BytecodeOptimizationConfig::noOptimization()
    {
        BytecodeOptimizationConfig cfg;
        cfg.enableLoopOptimization = false;
        cfg.enablePeephole = false;
        cfg.enableTrivialSetterInlining = false;
        cfg.enableTrivialGetterInlining = false;
        cfg.enableLocalArrayFusion = false;
        return cfg;
    }

    bool BytecodeOptimizationConfig::isPassEnabled(const std::string& passName) const
    {
        if (passName == "LoopOptimization")        return enableLoopOptimization;
        if (passName == "Peephole")                return enablePeephole;
        if (passName == "TrivialSetterInlining")   return enableTrivialSetterInlining;
        if (passName == "TrivialGetterInlining")   return enableTrivialGetterInlining;
        if (passName == "LocalArrayFusion")        return enableLocalArrayFusion;
        return true;
    }
}
