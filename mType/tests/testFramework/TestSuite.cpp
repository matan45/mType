#include "TestSuite.hpp"

namespace tests::testFramework
{
    TestSuite::TestSuite(const std::string& name)
        : suiteName(name)
    {
        runner = std::make_unique<TestRunner>(name);
    }

    void TestSuite::run()
    {
    }

    void TestSuite::generateReport()
    {
    }

    void TestSuite::addTestFromFile(const std::string& name, const std::string& filePath, TestType type)
    {
    }

    void TestSuite::generateHtmlReport()
    {
    }
}
