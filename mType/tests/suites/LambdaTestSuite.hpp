#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class LambdaTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/lambda/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/lambda/error/";
    public:
        explicit LambdaTestSuite() : TestSuite("Lambda Test Suite") {}
        void setupTests() override;
    };
}
