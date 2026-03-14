#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class StringInterpolationTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/interpolation/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/interpolation/error/";

    public:
        explicit StringInterpolationTestSuite() : TestSuite("String Interpolation Test Suite") {}

        void setupTests() override;
    };
}
