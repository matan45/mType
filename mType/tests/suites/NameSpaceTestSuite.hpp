#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class NameSpaceTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/nameSpace/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/nameSpace/error/";

    public:
        explicit NameSpaceTestSuite() : TestSuite("NameSpace Test Suite")
        {
        }

        void setupTests() override;
    };
}
