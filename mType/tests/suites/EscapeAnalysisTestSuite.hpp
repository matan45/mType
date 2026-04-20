#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    // MYT-134: verifies that enabling EscapeAnalysisPass + NEW_STACK path does
    // not alter observable program behavior. Covers allocations that should
    // promote (tight locals, loops, method calls on self, etc.) and allocations
    // that must stay on the heap (returned, stored into fields, captured by
    // lambdas, thrown, passed as arguments).
    class EscapeAnalysisTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath =
            "mType/tests/testFiles/escapeAnalysis/pass/";

    public:
        explicit EscapeAnalysisTestSuite() : TestSuite("Escape Analysis Test Suite") {}
        void setupTests() override;
    };
}
