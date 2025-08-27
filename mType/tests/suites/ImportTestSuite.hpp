#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class ImportTestSuite : public testFramework::TestSuite
    {
    public:
        explicit ImportTestSuite() : TestSuite("Import Test Suite") {}
        void setupTests() override;
    };
}
