#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class DependencyGraphTestSuite : public testFramework::TestSuite
    {
    public:
        explicit DependencyGraphTestSuite() : TestSuite("Dependency Graph Test Suite") {}
        void setupTests() override;
    };
}
