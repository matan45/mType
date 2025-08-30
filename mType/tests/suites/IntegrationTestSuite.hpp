#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class IntegrationTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/integration/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/integration/error/";
    public:
        explicit IntegrationTestSuite() : TestSuite("Integration Test Suite") {}
        void setupTests() override;
    };
}
