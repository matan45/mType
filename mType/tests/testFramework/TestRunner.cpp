#include "TestRunner.hpp"

namespace tests::testFramework
{
    TestRunner::TestRunner(const std::string& suiteName)
        : currentSuiteName(suiteName)
    {
    }

    TestRunner::~TestRunner()
    {
    }
}
