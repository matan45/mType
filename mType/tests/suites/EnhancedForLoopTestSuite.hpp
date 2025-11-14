#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class EnhancedForLoopTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/enhancedFor/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/enhancedFor/error/";

    public:
        explicit EnhancedForLoopTestSuite() : TestSuite("Enhanced For-Loop Test Suite") {}
        void setupTests() override;
    };
}
