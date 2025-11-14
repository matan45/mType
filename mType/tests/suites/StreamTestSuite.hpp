#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class StreamTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/stream/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/stream/error/";

    public:
        explicit StreamTestSuite() : TestSuite("Stream API Test Suite") {}
        void setupTests() override;
    };
}
