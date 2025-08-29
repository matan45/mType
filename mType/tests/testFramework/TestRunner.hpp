#pragma once
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "TestCase.hpp"

namespace tests::testFramework
{
    struct TestResults
    {
        int totalTests = 0;
        int passedTests = 0;
        int failedTests = 0;
        int errorTests = 0;
        int skippedTests = 0;
        std::chrono::milliseconds totalExecutionTime{0};
    };

    class TestRunner
    {
    private:
        std::string currentSuiteName;
        TestResults results;
        std::vector<TestCase*> executedTests;

    public:
        explicit TestRunner(const std::string& suiteName);
        ~TestRunner();

        void runTest(TestCase& testCase);
        void runTests(std::vector<TestCase>& testCases);
        
        const TestResults& getResults() const { return results; }
        const std::vector<TestCase*>& getExecutedTests() const { return executedTests; }
        
        void printSummary() const;
        void printDetailedReport() const;
    };
}
