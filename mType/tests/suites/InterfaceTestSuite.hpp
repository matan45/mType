#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class InterfaceTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/interface/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/interface/error/";
    public:
        explicit InterfaceTestSuite() : TestSuite("Interface Test Suite") {}
        void setupTests() override;
    };
}
