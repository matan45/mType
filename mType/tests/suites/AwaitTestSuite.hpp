#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class AwaitTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/await/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/await/error/";
    public:
        explicit AwaitTestSuite() : TestSuite("Await/Async Test Suite") {}
        void setupTests() override;
    };
}
