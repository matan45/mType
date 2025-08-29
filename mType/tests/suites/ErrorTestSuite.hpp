#pragma once
#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class ErrorTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/error/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/error/error/";
    public:
        explicit ErrorTestSuite() : TestSuite("Error Test Suite") {}
        void setupTests() override;
    };
}
