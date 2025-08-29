#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class ClassTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/class/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/class/error/";
    public:
        explicit ClassTestSuite() : TestSuite("Class Test Suite") {}
        void setupTests() override;
    };
}
