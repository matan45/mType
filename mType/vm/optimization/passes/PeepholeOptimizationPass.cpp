#include "PeepholeOptimizationPass.hpp"
#include "../base/BytecodeOptimizationContext.hpp"
#include "../PeepholeOptimizer.hpp"

#include <exception>
#include <iostream>

namespace vm::optimization::passes
{
    PeepholeOptimizationPass::PeepholeOptimizationPass()
        : BytecodePass("Peephole", base::PassType::TRANSFORMATION)
    {
    }

    void PeepholeOptimizationPass::optimize(bytecode::BytecodeProgram& program,
                                            base::BytecodeOptimizationContext& context)
    {
        auto cfg = PeepholeOptimizer::Config::forReleaseMode();
        PeepholeOptimizer peephole(cfg);
        peephole.registerDefaultPatterns();

        // Preserve the historic in-compile() swallow: peephole failures
        // log and continue rather than aborting the compile. The pass
        // manager's outer try/catch records the warning, but the original
        // behaviour also wrote to stderr — keep that for visibility.
        try
        {
            const bool modified = peephole.optimize(program);
            if (modified) recordTransformation();
            context.setModified(modified);
        }
        catch (const std::exception& e)
        {
            std::cerr << "WARNING: Peephole optimization failed: "
                      << e.what() << std::endl;
            std::cerr << "Continuing with unoptimized bytecode..." << std::endl;
        }
    }
}
