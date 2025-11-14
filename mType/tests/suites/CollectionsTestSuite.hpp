#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class CollectionsTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string passPath = "mType/tests/testFiles/collections/pass/";
        inline static const std::string errorPath = "mType/tests/testFiles/collections/error/";

    public:
        explicit CollectionsTestSuite() : TestSuite("Collections Test Suite") {}
        void setupTests() override;
    };
}
