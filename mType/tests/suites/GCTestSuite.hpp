#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    /**
     * @brief Test suite for Garbage Collection functionality
     *
     * Tests cycle detection and collection for various circular reference scenarios:
     * - Simple A -> B -> A cycles
     * - Self-referential objects
     * - Deep cycles (A -> B -> C -> D -> A)
     * - Lambda capture cycles
     * - Collection cycles
     */
    class GCTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/gc/pass/";
        // Note: GC tests don't have error cases - cycles should be handled gracefully

    public:
        explicit GCTestSuite() : TestSuite("Garbage Collection Test Suite") {}
        void setupTests() override;
    };
}
