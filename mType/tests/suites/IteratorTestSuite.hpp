#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class IteratorTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/iterator/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/iterator/error/";

    public:
        explicit IteratorTestSuite() : TestSuite("Iterator Test Suite") {}
        void setupTests() override;
    };
}
