#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    /**
     * Test suite for the pure-logic helpers the LSP completion handler
     * rides on (MYT-51). Covers what's testable without spinning up an
     * LSP harness — the dispatch handler itself will get coverage when
     * MYT-52 introduces a languageserver-tests project.
     *
     * Exercises:
     *   - util::findImportInsertLine         — import-line scan shared
     *                                          between CodeActionHandler
     *                                          and CompletionHandler
     *   - util::extractIdentifierTokenBefore — partial-token extraction
     *                                          under the cursor
     *   - The rustc-style fuzzy-filter budget
     *     (`max(1, typed.size() / 3)`) composed with util::levenshtein
     *
     * All tests run as NATIVE_CALLBACK (pure C++) so they live alongside
     * DiagnosticsTestSuite without needing .mt fixture files.
     */
    class CompletionLogicTestSuite : public testFramework::TestSuite
    {
    public:
        explicit CompletionLogicTestSuite()
            : TestSuite("Completion Logic Test Suite") {}

        void setupTests() override;
    };
}
