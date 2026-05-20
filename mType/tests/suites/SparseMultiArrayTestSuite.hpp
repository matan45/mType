#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class SparseMultiArrayTestSuite : public testFramework::TestSuite
    {
    public:
        explicit SparseMultiArrayTestSuite() : TestSuite("SparseMultiArray Test Suite") {}
        void setupTests() override;
    };
}
