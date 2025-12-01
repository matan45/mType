#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class ReflectionTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/reflection/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/reflection/error/";
    public:
        explicit ReflectionTestSuite() : TestSuite("Reflection Test Suite") {}
        void setupTests() override;
    };
}
