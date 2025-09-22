#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class ArrayTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/arrays/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/arrays/error/";

    public:
        explicit ArrayTestSuite() : TestSuite("Array Test Suite") {}
        void setupTests() override;
    };
}