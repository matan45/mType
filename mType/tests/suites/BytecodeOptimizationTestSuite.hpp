#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    /**
     * Proves the BytecodeOptimizer deepening: each post-AST bytecode pass
     * can be exercised against a hand-crafted BytecodeProgram in memory,
     * with no Environment, no VirtualMachine, and no .mt source. Before
     * the extraction, these passes were file-local helpers reachable only
     * through BytecodeCompiler::compile() — so a peephole-shape test
     * required writing source that compiled into the desired pattern.
     */
    class BytecodeOptimizationTestSuite : public testFramework::TestSuite
    {
    public:
        BytecodeOptimizationTestSuite()
            : TestSuite("Bytecode Optimization Test Suite") {}

        void setupTests() override;
    };
}
