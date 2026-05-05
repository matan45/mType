#include "TestRunner.hpp"
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

namespace tests::testFramework
{
    TestRunner::TestRunner(const std::string& suiteName)
        : currentSuiteName(suiteName)
    {
    }

    TestRunner::~TestRunner()
    {
    }

    void TestRunner::log(const std::string& message)
    {
        logBuffer << message;
        std::cout << message;
    }
    
    void TestRunner::logLine(const std::string& message)
    {
        logBuffer << message << std::endl;
        std::cout << message << std::endl;
    }

    void TestRunner::runTest(TestCase& testCase)
    {
        std::stringstream ss;
        ss << "  Running: " << std::left << std::setw(60) << testCase.getName() << " ... ";
        log(ss.str());
        std::cout.flush();
        
        testCase.execute();
        executedTests.push_back(&testCase);
        
        results.totalTests++;
        results.totalExecutionTime += testCase.getExecutionTime();
        
        std::stringstream statusStr;
        switch (testCase.getStatus())
        {
        case TestStatus::PASSED:
            results.passedTests++;
            log("\033[32mPASSED\033[0m");
            logBuffer << " [PASSED]";
            break;
        case TestStatus::FAILED:
            results.failedTests++;
            log("\033[31mFAILED\033[0m");
            logBuffer << " [FAILED]";
            break;
        case TestStatus::ERROR:
            results.errorTests++;
            log("\033[35mERROR\033[0m");
            logBuffer << " [ERROR]";
            break;
        case TestStatus::SKIPPED:
            results.skippedTests++;
            log("\033[33mSKIPPED\033[0m");
            logBuffer << " [SKIPPED]";
            break;
        default:
            log("UNKNOWN");
            logBuffer << " [UNKNOWN]";
            break;
        }
        
        statusStr << " (" << testCase.getExecutionTime().count() << "ms)";
        logLine(statusStr.str());
        
        if (testCase.getStatus() == TestStatus::FAILED || testCase.getStatus() == TestStatus::ERROR)
        {
            if (!testCase.getErrorMessage().empty())
            {
                logLine("    Error: " + testCase.getErrorMessage());
            }
        }
        
        // Log interpreter output if present
        if (!testCase.getOutput().empty())
        {
            logLine("    Interpreter Output:");
            std::istringstream outputStream(testCase.getOutput());
            std::string line;
            while (std::getline(outputStream, line))
            {
                logLine("      " + line);
            }
        }
    }

    void TestRunner::runTests(std::vector<TestCase>& testCases)
    {
        logLine();
        logLine(std::string(80, '='));
        logLine("Running Suite: " + currentSuiteName);
        logLine("Total Tests: " + std::to_string(testCases.size()));
        logLine(std::string(80, '-'));
        
        for (auto& testCase : testCases)
        {
            runTest(testCase);
        }
        
        printSummary();
    }

    void TestRunner::printSummary()
    {
        logLine(std::string(80, '-'));
        logLine("Suite Summary: " + currentSuiteName);
        logLine(std::string(80, '-'));
        
        // Calculate pass rate
        double passRate = (results.totalTests > 0) 
            ? (static_cast<double>(results.passedTests) / results.totalTests * 100.0) 
            : 0.0;
        
        // Print results with colors
        logLine("Total Tests: " + std::to_string(results.totalTests));
        
        log("\033[32mPassed: " + std::to_string(results.passedTests) + "\033[0m");
        logBuffer << " [Passed: " << results.passedTests << "]";
        logLine();
        
        if (results.failedTests > 0)
        {
            log("\033[31mFailed: " + std::to_string(results.failedTests) + "\033[0m");
            logBuffer << " [Failed: " << results.failedTests << "]";
            logLine();
        }
        else
            logLine("Failed: 0");
            
        if (results.errorTests > 0)
        {
            log("\033[35mErrors: " + std::to_string(results.errorTests) + "\033[0m");
            logBuffer << " [Errors: " << results.errorTests << "]";
            logLine();
        }
        else
            logLine("Errors: 0");
            
        if (results.skippedTests > 0)
        {
            log("\033[33mSkipped: " + std::to_string(results.skippedTests) + "\033[0m");
            logBuffer << " [Skipped: " << results.skippedTests << "]";
            logLine();
        }
        else
            logLine("Skipped: 0");
        
        std::stringstream ss;
        ss << "Pass Rate: " << std::fixed << std::setprecision(2) << passRate << "%";
        logLine(ss.str());
        logLine("Total Execution Time: " + std::to_string(results.totalExecutionTime.count()) + "ms");
        
        // Overall status
        if (results.failedTests == 0 && results.errorTests == 0)
        {
            logLine();
            log("\033[32m✓ Suite PASSED\033[0m");
            logBuffer << " [SUITE PASSED]";
            logLine();
        }
        else
        {
            logLine();
            log("\033[31m✗ Suite FAILED\033[0m");
            logBuffer << " [SUITE FAILED]";
            logLine();
        }
        
        logLine(std::string(80, '='));
        logLine();
    }

    void TestRunner::printDetailedReport()
    {
        logLine();
        logLine(std::string(80, '='));
        logLine("Detailed Test Report: " + currentSuiteName);
        logLine(std::string(80, '='));
        
        // Group tests by status
        logLine();
        log("\033[32mPASSED TESTS:\033[0m");
        logBuffer << " [PASSED TESTS]";
        logLine();
        for (const auto* test : executedTests)
        {
            if (test->getStatus() == TestStatus::PASSED)
            {
                std::stringstream ss;
                ss << "  ✓ " << test->getName() 
                   << " (" << test->getExecutionTime().count() << "ms)";
                logLine(ss.str());
                
                // Include output for passed tests if present
                if (!test->getOutput().empty())
                {
                    logLine("    Interpreter Output:");
                    std::istringstream outputStream(test->getOutput());
                    std::string outputLine;
                    while (std::getline(outputStream, outputLine))
                    {
                        logLine("      " + outputLine);
                    }
                }
            }
        }
        
        if (results.failedTests > 0)
        {
            logLine();
            log("\033[31mFAILED TESTS:\033[0m");
            logBuffer << " [FAILED TESTS]";
            logLine();
            for (const auto* test : executedTests)
            {
                if (test->getStatus() == TestStatus::FAILED)
                {
                    logLine("  ✗ " + test->getName());
                    logLine("    File: " + test->getFilePath());
                    logLine("    Error: " + test->getErrorMessage());
                    
                    // Include output for failed tests if present
                    if (!test->getOutput().empty())
                    {
                        logLine("    Interpreter Output:");
                        std::istringstream outputStream(test->getOutput());
                        std::string outputLine;
                        while (std::getline(outputStream, outputLine))
                        {
                            logLine("      " + outputLine);
                        }
                    }
                }
            }
        }
        
        if (results.errorTests > 0)
        {
            logLine();
            log("\033[35mERROR TESTS:\033[0m");
            logBuffer << " [ERROR TESTS]";
            logLine();
            for (const auto* test : executedTests)
            {
                if (test->getStatus() == TestStatus::ERROR)
                {
                    logLine("  ! " + test->getName());
                    logLine("    File: " + test->getFilePath());
                    logLine("    Error: " + test->getErrorMessage());
                    
                    // Include output for error tests if present
                    if (!test->getOutput().empty())
                    {
                        logLine("    Interpreter Output:");
                        std::istringstream outputStream(test->getOutput());
                        std::string outputLine;
                        while (std::getline(outputStream, outputLine))
                        {
                            logLine("      " + outputLine);
                        }
                    }
                }
            }
        }
        
        logLine(std::string(80, '='));
    }
    
    void TestRunner::saveLogToFile(const std::string& filename)
    {
        std::ofstream file(filename);
        if (file.is_open())
        {
            // Remove ANSI color codes from log content for file output
            std::string content = logBuffer.str();
            std::string cleanContent;
            bool inEscape = false;
            
            for (size_t i = 0; i < content.length(); ++i)
            {
                if (content[i] == '\033')
                {
                    inEscape = true;
                }
                else if (inEscape && content[i] == 'm')
                {
                    inEscape = false;
                }
                else if (!inEscape)
                {
                    cleanContent += content[i];
                }
            }
            
            file << cleanContent;
            file.close();
            std::cout << "Log saved to: " << filename << std::endl;
        }
        else
        {
            std::cerr << "Failed to create log file: " << filename << std::endl;
        }
    }
}
