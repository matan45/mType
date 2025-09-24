#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    /**
     * @brief Test suite for StringPool functionality and integration
     *
     * Comprehensive test suite that validates:
     * - Basic string pool functionality
     * - Memory efficiency and deduplication
     * - Performance improvements
     * - Integration with all mType language features
     * - Thread safety (implicitly through framework)
     * - Edge cases and stress scenarios
     */
    class StringPoolTestSuite : public testFramework::TestSuite
    {
    private:
        // Test file paths for string pool specific tests
        inline static const std::string stringPoolPath = "mType/tests/testFiles/stringpool/";

    public:
        explicit StringPoolTestSuite() : TestSuite("StringPool Test Suite") {}

        /**
         * @brief Setup all StringPool test cases
         *
         * Organizes tests into categories:
         * - Basic functionality tests
         * - Efficiency and deduplication tests
         * - Performance and stress tests
         * - Integration tests with language features
         * - Edge case and boundary tests
         */
        void setupTests() override;
    };
}