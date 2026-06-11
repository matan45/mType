#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class NumericsTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/numerics/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/numerics/error/";
    public:
        explicit NumericsTestSuite() : TestSuite("Numerics and Bitwise Test Suite") {}
        void setupTests() override;
    };
}
