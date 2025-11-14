#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class OverloadingTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/overloading/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/overloading/error/";
    public:
        explicit OverloadingTestSuite() : TestSuite("Overloading Test Suite") {}
        void setupTests() override;
    };
}
