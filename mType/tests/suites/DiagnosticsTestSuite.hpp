#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    /**
     * Test suite for the diagnostics foundation introduced by MYT-35.
     *
     * Exercises pure-C++ utilities (no .mt source required):
     *   - util::levenshtein                  — edit distance with early-out
     *   - util::findClosestMatch             — rustc-style "did you mean" picker
     *   - diagnostics::SourceFileCache       — publish / lookup / invalidate / normalize
     *
     * All tests run as NATIVE_CALLBACK so they live alongside the rest of
     * the test suite without needing .mt fixture files.
     */
    class DiagnosticsTestSuite : public testFramework::TestSuite
    {
    public:
        explicit DiagnosticsTestSuite() : TestSuite("Diagnostics Test Suite") {}

        void setupTests() override;
    };
}
