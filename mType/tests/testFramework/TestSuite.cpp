#include "TestSuite.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>

namespace tests::testFramework
{
    TestSuite::TestSuite(const std::string& name)
        : suiteName(name)
    {
        runner = std::make_unique<TestRunner>(name);
    }

    void TestSuite::run()
    {
        // Setup tests if not already done
        if (testCases.empty())
        {
            setupTests();
        }
        
        // Run all tests through the runner
        runner->runTests(testCases);
        
        // Generate reports
        generateReport();
    }

    void TestSuite::setExecutionModeForAll(constants::ExecutionMode mode)
    {
        // Execution mode is always BYTECODE_VM - this method is now a no-op
        // Kept for backwards compatibility
    }

    void TestSuite::generateReport()
    {
        // Generate both console and HTML reports
        runner->printDetailedReport();
        generateHtmlReport();

        // Save log file
        std::filesystem::create_directories("test_logs");
        
        // Generate log filename with timestamp
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << "test_logs/" << suiteName << "_"
            << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".log";
        std::string logFilename = oss.str();
        
        // Replace spaces with underscores in filename
        for (char& c : logFilename)
        {
            if (c == ' ') c = '_';
        }
        
        runner->saveLogToFile(logFilename);
    }

    void TestSuite::addTestFromFile(const std::string& name, const std::string& filePath, TestType type)
    {
        testCases.emplace_back(name, filePath, type);
    }

    void TestSuite::addOutputVerificationTest(const std::string& name, const std::string& filePath)
    {
        testCases.emplace_back(name, filePath, TestType::OUTPUT_EXPECTED);
    }

    void TestSuite::addInteropTest(const std::string& name, const std::string& filePath)
    {
        testCases.emplace_back(name, filePath, TestType::SCRIPT_INTEROP);
    }

    void TestSuite::generateHtmlReport()
    {
        // Create reports directory if it doesn't exist
        std::filesystem::create_directories("test_reports");
        
        // Generate filename with timestamp
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << "test_reports/" << suiteName << "_"
            << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".html";
        std::string filename = oss.str();
        
        // Replace spaces with underscores in filename
        for (char& c : filename)
        {
            if (c == ' ') c = '_';
        }
        
        std::ofstream htmlFile(filename);
        if (!htmlFile.is_open())
        {
            std::cerr << "Failed to create HTML report file: " << filename << std::endl;
            return;
        }
        
        const auto& results = runner->getResults();
        const auto& executedTests = runner->getExecutedTests();
        
        // Calculate pass rate
        double passRate = (results.totalTests > 0) 
            ? (static_cast<double>(results.passedTests) / results.totalTests * 100.0) 
            : 0.0;
        
        // Write HTML header
        htmlFile << "<!DOCTYPE html>\n";
        htmlFile << "<html lang=\"en\">\n";
        htmlFile << "<head>\n";
        htmlFile << "    <meta charset=\"UTF-8\">\n";
        htmlFile << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        htmlFile << "    <title>" << suiteName << " - Test Report</title>\n";
        htmlFile << "    <style>\n";
        htmlFile << "        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }\n";
        htmlFile << "        .container { max-width: 1200px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
        htmlFile << "        h1 { color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }\n";
        htmlFile << "        h2 { color: #555; margin-top: 30px; }\n";
        htmlFile << "        .summary { display: flex; justify-content: space-around; margin: 20px 0; }\n";
        htmlFile << "        .summary-card { text-align: center; padding: 15px; border-radius: 5px; flex: 1; margin: 0 10px; }\n";
        htmlFile << "        .passed { background-color: #d4edda; color: #155724; }\n";
        htmlFile << "        .failed { background-color: #f8d7da; color: #721c24; }\n";
        htmlFile << "        .error { background-color: #e7d4f8; color: #4a1a5c; }\n";
        htmlFile << "        .skipped { background-color: #fff3cd; color: #856404; }\n";
        htmlFile << "        .info { background-color: #d1ecf1; color: #0c5460; }\n";
        htmlFile << "        .test-table { width: 100%; border-collapse: collapse; margin-top: 20px; }\n";
        htmlFile << "        .test-table th, .test-table td { padding: 10px; text-align: left; border: 1px solid #ddd; }\n";
        htmlFile << "        .test-table th { background-color: #f2f2f2; font-weight: bold; }\n";
        htmlFile << "        .test-table tr:nth-child(even) { background-color: #f9f9f9; }\n";
        htmlFile << "        .status-passed { color: #28a745; font-weight: bold; }\n";
        htmlFile << "        .status-failed { color: #dc3545; font-weight: bold; }\n";
        htmlFile << "        .status-error { color: #6f42c1; font-weight: bold; }\n";
        htmlFile << "        .status-skipped { color: #ffc107; font-weight: bold; }\n";
        htmlFile << "        .timestamp { color: #666; font-size: 0.9em; margin-top: 10px; }\n";
        htmlFile << "        .progress-bar { width: 100%; height: 30px; background-color: #e0e0e0; border-radius: 15px; overflow: hidden; margin: 20px 0; }\n";
        htmlFile << "        .progress-fill { height: 100%; background-color: #4CAF50; text-align: center; line-height: 30px; color: white; }\n";
        htmlFile << "        .error-message { color: #dc3545; font-style: italic; font-size: 0.9em; }\n";
        htmlFile << "    </style>\n";
        htmlFile << "</head>\n";
        htmlFile << "<body>\n";
        htmlFile << "    <div class=\"container\">\n";
        
        // Title and timestamp
        htmlFile << "        <h1>" << suiteName << " - Test Report</h1>\n";
        htmlFile << "        <div class=\"timestamp\">Generated: " 
                 << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "</div>\n";
        
        // Progress bar
        htmlFile << "        <div class=\"progress-bar\">\n";
        htmlFile << "            <div class=\"progress-fill\" style=\"width: " << passRate << "%;\">\n";
        htmlFile << "                " << std::fixed << std::setprecision(1) << passRate << "% Passed\n";
        htmlFile << "            </div>\n";
        htmlFile << "        </div>\n";
        
        // Summary cards
        htmlFile << "        <div class=\"summary\">\n";
        htmlFile << "            <div class=\"summary-card info\">\n";
        htmlFile << "                <h3>Total Tests</h3>\n";
        htmlFile << "                <h2>" << results.totalTests << "</h2>\n";
        htmlFile << "            </div>\n";
        htmlFile << "            <div class=\"summary-card passed\">\n";
        htmlFile << "                <h3>Passed</h3>\n";
        htmlFile << "                <h2>" << results.passedTests << "</h2>\n";
        htmlFile << "            </div>\n";
        htmlFile << "            <div class=\"summary-card failed\">\n";
        htmlFile << "                <h3>Failed</h3>\n";
        htmlFile << "                <h2>" << results.failedTests << "</h2>\n";
        htmlFile << "            </div>\n";
        htmlFile << "            <div class=\"summary-card error\">\n";
        htmlFile << "                <h3>Errors</h3>\n";
        htmlFile << "                <h2>" << results.errorTests << "</h2>\n";
        htmlFile << "            </div>\n";
        htmlFile << "            <div class=\"summary-card skipped\">\n";
        htmlFile << "                <h3>Skipped</h3>\n";
        htmlFile << "                <h2>" << results.skippedTests << "</h2>\n";
        htmlFile << "            </div>\n";
        htmlFile << "        </div>\n";
        
        // Execution time
        htmlFile << "        <p><strong>Total Execution Time:</strong> " 
                 << results.totalExecutionTime.count() << "ms</p>\n";
        
        // Test details table
        htmlFile << "        <h2>Test Details</h2>\n";
        htmlFile << "        <table class=\"test-table\">\n";
        htmlFile << "            <thead>\n";
        htmlFile << "                <tr>\n";
        htmlFile << "                    <th>#</th>\n";
        htmlFile << "                    <th>Test Name</th>\n";
        htmlFile << "                    <th>Status</th>\n";
        htmlFile << "                    <th>Type</th>\n";
        htmlFile << "                    <th>Time (ms)</th>\n";
        htmlFile << "                    <th>File Path</th>\n";
        htmlFile << "                    <th>Error Message</th>\n";
        htmlFile << "                </tr>\n";
        htmlFile << "            </thead>\n";
        htmlFile << "            <tbody>\n";
        
        int testNum = 1;
        for (const auto* test : executedTests)
        {
            htmlFile << "                <tr>\n";
            htmlFile << "                    <td>" << testNum++ << "</td>\n";
            htmlFile << "                    <td>" << test->getName() << "</td>\n";
            
            // Status with color
            htmlFile << "                    <td class=\"";
            switch (test->getStatus())
            {
            case TestStatus::PASSED:
                htmlFile << "status-passed\">PASSED";
                break;
            case TestStatus::FAILED:
                htmlFile << "status-failed\">FAILED";
                break;
            case TestStatus::ERROR:
                htmlFile << "status-error\">ERROR";
                break;
            case TestStatus::SKIPPED:
                htmlFile << "status-skipped\">SKIPPED";
                break;
            default:
                htmlFile << "\">UNKNOWN";
                break;
            }
            htmlFile << "</td>\n";
            
            htmlFile << "                    <td>" << TestCase::typeToString(test->getType()) << "</td>\n";
            htmlFile << "                    <td>" << test->getExecutionTime().count() << "</td>\n";
            htmlFile << "                    <td>" << test->getFilePath() << "</td>\n";
            htmlFile << "                    <td class=\"error-message\">" 
                     << (test->getErrorMessage().empty() ? "-" : test->getErrorMessage()) 
                     << "</td>\n";
            htmlFile << "                </tr>\n";
        }
        
        htmlFile << "            </tbody>\n";
        htmlFile << "        </table>\n";
        
        htmlFile << "    </div>\n";
        htmlFile << "</body>\n";
        htmlFile << "</html>\n";
        
        htmlFile.close();
        
        std::cout << "HTML report generated: " << filename << std::endl;
    }
}
