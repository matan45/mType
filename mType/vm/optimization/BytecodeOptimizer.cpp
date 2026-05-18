#include "BytecodeOptimizer.hpp"

namespace vm::optimization
{
    BytecodeOptimizer::BytecodeOptimizer(const BytecodeOptimizationConfig& cfg)
        : passManager(std::make_unique<BytecodeOptimizationPassManager>(cfg))
        , config(cfg)
    {
        passManager->registerDefaultPasses();
    }

    BytecodeOptimizer::~BytecodeOptimizer() = default;

    BytecodeOptimizationResult BytecodeOptimizer::optimize(bytecode::BytecodeProgram& program)
    {
        return passManager->runPasses(program);
    }

    void BytecodeOptimizer::setConfig(const BytecodeOptimizationConfig& cfg)
    {
        config = cfg;
        passManager->setConfig(cfg);
    }

    void BytecodeOptimizer::enablePass(const std::string& passName)
    {
        passManager->enablePass(passName);
    }

    void BytecodeOptimizer::disablePass(const std::string& passName)
    {
        passManager->disablePass(passName);
    }

    const BytecodeOptimizationResult& BytecodeOptimizer::getLastResult() const
    {
        return passManager->getLastResult();
    }
}
