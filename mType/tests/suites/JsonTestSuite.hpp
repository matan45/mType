#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class JsonTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/json/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/json/error/";
    public:
        explicit JsonTestSuite() : TestSuite("JSON Test Suite") {}
        void setupTests() override;
    };
}
