#pragma once
#include "../testFramework/TestSuite.hpp"

namespace tests::testSuite
{
    class ExeTestSuite : public testFramework::TestSuite
    {
    private:
        inline static const std::string basePath = "mType/tests/testFiles/exe/";
    public:
        explicit ExeTestSuite() : TestSuite("Exe Build Test Suite") {}
        void setupTests() override;
    };
}
