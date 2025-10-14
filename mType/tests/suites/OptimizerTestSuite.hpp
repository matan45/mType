#pragma once
#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    /**
     * Test suite for AST optimization passes.
     * Tests dead code elimination and other optimizations.
     */
    class OptimizerTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/optimizer/pass/";
    public:
        explicit OptimizerTestSuite() : TestSuite("Optimizer Test Suite") {}
        void setupTests() override;
    };
}
