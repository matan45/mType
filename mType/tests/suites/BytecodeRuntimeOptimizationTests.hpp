#pragma once

namespace tests::testSuite
{
    class BytecodeOptimizationTestSuite;

    // Runtime/JIT regression registrations are kept out of the bytecode-pass
    // suite implementation so both translation units stay focused and under
    // the project's normal 500-line implementation-file limit.
    void registerBytecodeRuntimeOptimizationTests(
        BytecodeOptimizationTestSuite& suite);
}
