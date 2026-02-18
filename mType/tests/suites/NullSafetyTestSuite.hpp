#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class NullSafetyTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/nullSafety/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/nullSafety/error/";
    public:
        explicit NullSafetyTestSuite() : TestSuite("Null Safety Test Suite") {}
        void setupTests() override;
    };
}
