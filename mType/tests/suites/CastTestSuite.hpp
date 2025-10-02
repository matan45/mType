#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class CastTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/cast/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/cast/error/";
    public:
        explicit CastTestSuite() : TestSuite("Cast and Type Checking Test Suite") {}
        void setupTests() override;
    };
}
