#pragma once
#include  <string>

namespace tests::testFramework
{
    class TestRunner
    {
    private:
        std::string currentSuiteName;

    public:
        explicit TestRunner(const std::string& suiteName);
        ~TestRunner();
    };
}
