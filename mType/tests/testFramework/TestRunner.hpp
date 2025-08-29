#pragma once
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <fstream>
#include <sstream>
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
        std::stringstream logBuffer;
        std::ofstream logFile;

    public:
        explicit TestRunner(const std::string& suiteName);
        ~TestRunner();

        void runTest(TestCase& testCase);
        void runTests(std::vector<TestCase>& testCases);
        
        const TestResults& getResults() const { return results; }
        const std::vector<TestCase*>& getExecutedTests() const { return executedTests; }
        const std::string getLogContent() const { return logBuffer.str(); }
        
        void printSummary();
        void printDetailedReport();
        void saveLogToFile(const std::string& filename);
        
    private:
        void log(const std::string& message);
        void logLine(const std::string& message = "");
    };
}
