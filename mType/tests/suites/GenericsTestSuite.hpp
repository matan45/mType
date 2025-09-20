#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class GenericsTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/generics/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/generics/error/";
    public:
        explicit GenericsTestSuite() : TestSuite("Generics Test Suite") {}
        void setupTests() override;
    };
}