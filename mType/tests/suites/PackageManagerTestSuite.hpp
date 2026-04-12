#pragma once

#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class PackageManagerTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string fixturePath = "mType/tests/testFiles/packagemanager/";
    public:
        explicit PackageManagerTestSuite() : TestSuite("Package Manager Test Suite") {}
        void setupTests() override;
    };
}
