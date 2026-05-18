#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    /**
     * Proves the ExecutionContext deepening: a frame-local executor
     * (StackOperationsExecutor) can be constructed and exercised against a
     * minimal ExecutionContext — no Environment, no VirtualMachine. Before
     * the deepening, ExecutionContext's constructor required a shared
     * Environment + VM back-pointer, so this test was structurally
     * impossible.
     */
    class ExecutorIsolationTestSuite : public testFramework::TestSuite
    {
    public:
        ExecutorIsolationTestSuite() : TestSuite("Executor Isolation Test Suite") {}

        void setupTests() override;
    };
}
