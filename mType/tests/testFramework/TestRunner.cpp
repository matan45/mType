#include "TestRunner.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace tests::testFramework
{
    TestRunner::TestRunner(const std::string& suiteName)
        : currentSuiteName(suiteName)
    {
    }

    TestRunner::~TestRunner()
    {
    }

    void TestRunner::runTest(TestCase& testCase)
    {
        std::cout << "  Running: " << std::left << std::setw(60) << testCase.getName() << " ... ";
        std::cout.flush();
        
        testCase.execute();
        executedTests.push_back(&testCase);
        
        results.totalTests++;
        results.totalExecutionTime += testCase.getExecutionTime();
        
        switch (testCase.getStatus())
        {
        case TestStatus::PASSED:
            results.passedTests++;
            std::cout << "\033[32mPASSED\033[0m";
            break;
        case TestStatus::FAILED:
            results.failedTests++;
            std::cout << "\033[31mFAILED\033[0m";
            break;
        case TestStatus::ERROR:
            results.errorTests++;
            std::cout << "\033[35mERROR\033[0m";
            break;
        case TestStatus::SKIPPED:
            results.skippedTests++;
            std::cout << "\033[33mSKIPPED\033[0m";
            break;
        default:
            std::cout << "UNKNOWN";
            break;
        }
        
        std::cout << " (" << testCase.getExecutionTime().count() << "ms)" << std::endl;
        
        if (testCase.getStatus() == TestStatus::FAILED || testCase.getStatus() == TestStatus::ERROR)
        {
            if (!testCase.getErrorMessage().empty())
            {
                std::cout << "    Error: " << testCase.getErrorMessage() << std::endl;
            }
        }
    }

    void TestRunner::runTests(std::vector<TestCase>& testCases)
    {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "Running Suite: " << currentSuiteName << std::endl;
        std::cout << "Total Tests: " << testCases.size() << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        for (auto& testCase : testCases)
        {
            runTest(testCase);
        }
        
        printSummary();
    }

    void TestRunner::printSummary() const
    {
        std::cout << std::string(80, '-') << std::endl;
        std::cout << "Suite Summary: " << currentSuiteName << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        // Calculate pass rate
        double passRate = (results.totalTests > 0) 
            ? (static_cast<double>(results.passedTests) / results.totalTests * 100.0) 
            : 0.0;
        
        // Print results with colors
        std::cout << "Total Tests: " << results.totalTests << std::endl;
        std::cout << "\033[32mPassed: " << results.passedTests << "\033[0m" << std::endl;
        
        if (results.failedTests > 0)
            std::cout << "\033[31mFailed: " << results.failedTests << "\033[0m" << std::endl;
        else
            std::cout << "Failed: 0" << std::endl;
            
        if (results.errorTests > 0)
            std::cout << "\033[35mErrors: " << results.errorTests << "\033[0m" << std::endl;
        else
            std::cout << "Errors: 0" << std::endl;
            
        if (results.skippedTests > 0)
            std::cout << "\033[33mSkipped: " << results.skippedTests << "\033[0m" << std::endl;
        else
            std::cout << "Skipped: 0" << std::endl;
        
        std::cout << "Pass Rate: " << std::fixed << std::setprecision(2) << passRate << "%" << std::endl;
        std::cout << "Total Execution Time: " << results.totalExecutionTime.count() << "ms" << std::endl;
        
        // Overall status
        if (results.failedTests == 0 && results.errorTests == 0)
        {
            std::cout << "\n\033[32m✓ Suite PASSED\033[0m" << std::endl;
        }
        else
        {
            std::cout << "\n\033[31m✗ Suite FAILED\033[0m" << std::endl;
        }
        
        std::cout << std::string(80, '=') << std::endl << std::endl;
    }

    void TestRunner::printDetailedReport() const
    {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "Detailed Test Report: " << currentSuiteName << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        // Group tests by status
        std::cout << "\n\033[32mPASSED TESTS:\033[0m" << std::endl;
        for (const auto* test : executedTests)
        {
            if (test->getStatus() == TestStatus::PASSED)
            {
                std::cout << "  ✓ " << test->getName() 
                          << " (" << test->getExecutionTime().count() << "ms)" << std::endl;
            }
        }
        
        if (results.failedTests > 0)
        {
            std::cout << "\n\033[31mFAILED TESTS:\033[0m" << std::endl;
            for (const auto* test : executedTests)
            {
                if (test->getStatus() == TestStatus::FAILED)
                {
                    std::cout << "  ✗ " << test->getName() << std::endl;
                    std::cout << "    File: " << test->getFilePath() << std::endl;
                    std::cout << "    Error: " << test->getErrorMessage() << std::endl;
                }
            }
        }
        
        if (results.errorTests > 0)
        {
            std::cout << "\n\033[35mERROR TESTS:\033[0m" << std::endl;
            for (const auto* test : executedTests)
            {
                if (test->getStatus() == TestStatus::ERROR)
                {
                    std::cout << "  ! " << test->getName() << std::endl;
                    std::cout << "    File: " << test->getFilePath() << std::endl;
                    std::cout << "    Error: " << test->getErrorMessage() << std::endl;
                }
            }
        }
        
        std::cout << std::string(80, '=') << std::endl;
    }
}
