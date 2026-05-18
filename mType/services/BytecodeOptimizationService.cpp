#include "BytecodeOptimizationService.hpp"
#include "../vm/optimization/BytecodeOptimizer.hpp"

namespace services
{
    BytecodeOptimizationService::BytecodeOptimizationService()
        : optimizer(std::make_unique<vm::optimization::BytecodeOptimizer>(
              vm::optimization::BytecodeOptimizationConfig::forRelease()))
    {
    }

    BytecodeOptimizationService::~BytecodeOptimizationService() = default;

    vm::optimization::BytecodeOptimizationResult
    BytecodeOptimizationService::optimize(vm::bytecode::BytecodeProgram& program)
    {
        return optimizer->optimize(program);
    }
}
