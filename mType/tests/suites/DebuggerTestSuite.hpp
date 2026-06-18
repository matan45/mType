#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class DebuggerTestSuite : public testFramework::TestSuite
    {
    public:
        explicit DebuggerTestSuite() : TestSuite("Debugger Test Suite")
        {
        }

        void setupTests() override;
    };
}
