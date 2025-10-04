#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class ModifiersTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/modifiers/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/modifiers/error/";
    public:
        explicit ModifiersTestSuite() : TestSuite("Access Modifiers Test Suite") {}
        void setupTests() override;
    };
}
