#pragma once
#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class ControlFlowTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "tests/testFiles/controlFlow/pass/";
        inline static const std::string errorPath = "tests/testFiles/controlFlow/error/";
    public:
        explicit ControlFlowTestSuite() : TestSuite("Control Flow Test Suite") {}
        void setupTests() override;
    }; 
}
