#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class TypeCheckingTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "tests/testFiles/typeChecking/pass/";
        inline static const std::string errorPath = "tests/testFiles/typeChecking/error/";

    public:
        explicit TypeCheckingTestSuite() : TestSuite("Type Checking Test Suite")
        {
        }

        void setupTests() override;
    };
};
