#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class IntegrationTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/integration/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/integration/error/";
        // MYT-259: benchmark scripts double as JIT-correctness regression
        // tests. Each .mt has a sibling .expected captured from a `--no-jit`
        // reference run; running with JIT on must produce the same output.
        inline static const std::string benchmarkPath = "mType/tests/testFiles/benchmarks/";
    public:
        explicit IntegrationTestSuite() : TestSuite("Integration Test Suite") {}
        void setupTests() override;
    };
}
