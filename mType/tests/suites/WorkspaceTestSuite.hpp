#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class WorkspaceTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string fixturePath = "mType/tests/testFiles/workspace/";
    public:
        explicit WorkspaceTestSuite() : TestSuite("Workspace Test Suite") {}
        void setupTests() override;
    };
}
