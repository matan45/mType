#include "TestCase.hpp"
#include "../../parser/Parser.hpp"
#include "../../lexer/Lexer.hpp"
#include "../../services/ImportManager.hpp"
#include "../../evaluator/Evaluator.hpp"
#include "../../environment/EnvironmentBuilder.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/ParseException.hpp"
#include "../../evaluator/utils/GenericTypeManager.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace tests::testFramework
{
    using namespace parser;
    using namespace lexer;
    using namespace services;
    using namespace evaluator;
    using namespace environment;

    TestCase::TestCase(const std::string& testName, const std::string& testFilePath, TestType testType)
        : name(testName), filePath(testFilePath), type(testType), status(TestStatus::NOT_RUN),
          executionTime(0)
    {
    }

    void TestCase::execute()
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        // Clear generic class cache to prevent contamination between tests
        evaluator::utils::GenericTypeManager::clearGenericClassCache();

        try
        {
            // Check if file exists
            if (!std::filesystem::exists(filePath))
            {
                status = TestStatus::ERROR;
                errorMessage = "Test file not found: " + filePath;
                auto endTime = std::chrono::high_resolution_clock::now();
                executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                return;
            }

            // Redirect stdout to capture output
            std::stringstream outputBuffer;
            std::streambuf* oldCout = std::cout.rdbuf();
            std::cout.rdbuf(outputBuffer.rdbuf());

            try
            {
                // Initialize lexer and parser - let lexer read the file
                Lexer lexer(filePath);
                auto importManager = std::make_unique<ImportManager>();
                ImportManager* importManagerPtr = importManager.get();
                
                // Set base directory to the directory of the test file
                std::filesystem::path testFilePath(filePath);
                importManagerPtr->setBaseDirectory(testFilePath.parent_path().string());
                
                Parser parser(lexer, std::move(importManager));

                // Parse the file
                auto ast = parser.parseProgram();

                if (!ast)
                {
                    throw std::runtime_error("Failed to generate AST");
                }

                // Evaluate the AST
                auto env = EnvironmentBuilder::createDefault();
                
                // Set ImportManager on environment for clean architecture
                env->setImportManager(importManagerPtr);
                
                Evaluator evaluator(env);
                evaluator.evaluate(ast.get());

                // Restore stdout
                std::cout.rdbuf(oldCout);
                output = outputBuffer.str();

                // Determine test result based on type
                if (type == TestType::ERROR_EXPECTED)
                {
                    // This test was expected to fail but passed
                    status = TestStatus::FAILED;
                    errorMessage = "Expected error but test passed";
                }
                else if (type == TestType::OUTPUT_EXPECTED)
                {
                    // Verify output against expected file
                    if (verifyOutputAgainstExpected())
                    {
                        status = TestStatus::PASSED;
                    }
                    else
                    {
                        status = TestStatus::FAILED;
                        errorMessage = "Output does not match expected result";
                    }
                }
                else
                {
                    // Normal test passed
                    status = TestStatus::PASSED;
                }
            }
            catch (const errors::ParseException& e)
            {
                std::cout.rdbuf(oldCout);
                output = outputBuffer.str();
                
                if (type == TestType::ERROR_EXPECTED)
                {
                    status = TestStatus::PASSED;
                    errorMessage = "Expected parse error: " + std::string(e.what());
                }
                else
                {
                    status = TestStatus::FAILED;
                    errorMessage = "Parse error: " + std::string(e.what());
                }
            }
            catch (const errors::TypeException& e)
            {
                std::cout.rdbuf(oldCout);
                output = outputBuffer.str();
                
                if (type == TestType::ERROR_EXPECTED)
                {
                    status = TestStatus::PASSED;
                    errorMessage = "Expected type error: " + std::string(e.what());
                }
                else
                {
                    status = TestStatus::FAILED;
                    errorMessage = "Type error: " + std::string(e.what());
                }
            }
            catch (const errors::UndefinedException& e)
            {
                std::cout.rdbuf(oldCout);
                output = outputBuffer.str();
                
                if (type == TestType::ERROR_EXPECTED)
                {
                    status = TestStatus::PASSED;
                    errorMessage = "Expected undefined error: " + std::string(e.what());
                }
                else
                {
                    status = TestStatus::FAILED;
                    errorMessage = "Undefined error: " + std::string(e.what());
                }
            }
            catch (const std::exception& e)
            {
                std::cout.rdbuf(oldCout);
                output = outputBuffer.str();
                
                if (type == TestType::ERROR_EXPECTED)
                {
                    status = TestStatus::PASSED;
                    errorMessage = "Expected error: " + std::string(e.what());
                }
                else
                {
                    status = TestStatus::FAILED;
                    errorMessage = "Runtime error: " + std::string(e.what());
                }
            }
        }
        catch (const std::exception& e)
        {
            status = TestStatus::ERROR;
            errorMessage = "Test execution error: " + std::string(e.what());
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    }

    std::string TestCase::statusToString(TestStatus status)
    {
        switch (status)
        {
        case TestStatus::NOT_RUN:
            return "NOT RUN";
        case TestStatus::PASSED:
            return "PASSED";
        case TestStatus::FAILED:
            return "FAILED";
        case TestStatus::ERROR:
            return "ERROR";
        case TestStatus::SKIPPED:
            return "SKIPPED";
        default:
            return "UNKNOWN";
        }
    }

    std::string TestCase::typeToString(TestType type)
    {
        switch (type)
        {
        case TestType::NORMAL:
            return "NORMAL";
        case TestType::ERROR_EXPECTED:
            return "ERROR_EXPECTED";
        case TestType::OUTPUT_EXPECTED:
            return "OUTPUT_EXPECTED";
        case TestType::PERFORMANCE:
            return "PERFORMANCE";
        default:
            return "UNKNOWN";
        }
    }

    bool TestCase::verifyOutputAgainstExpected() const
    {
        // Construct expected output file path by replacing .mt with .expected
        std::string expectedFilePath = filePath;
        size_t pos = expectedFilePath.find_last_of('.');
        if (pos != std::string::npos) {
            expectedFilePath = expectedFilePath.substr(0, pos) + ".expected";
        } else {
            expectedFilePath += ".expected";
        }

        // Check if expected file exists
        if (!std::filesystem::exists(expectedFilePath)) {
            return false; // Cannot verify without expected file
        }

        // Read expected output
        std::ifstream expectedFile(expectedFilePath);
        if (!expectedFile.is_open()) {
            return false;
        }

        std::string expectedOutput((std::istreambuf_iterator<char>(expectedFile)),
                                   std::istreambuf_iterator<char>());
        expectedFile.close();

        // Normalize line endings and whitespace for comparison
        auto normalize = [](std::string str) {
            // Remove trailing whitespace from each line
            std::stringstream ss(str);
            std::string line;
            std::string result;
            while (std::getline(ss, line)) {
                // Remove trailing whitespace
                while (!line.empty() && std::isspace(line.back())) {
                    line.pop_back();
                }
                if (!result.empty()) {
                    result += "\n";
                }
                result += line;
            }
            return result;
        };

        std::string normalizedOutput = normalize(output);
        std::string normalizedExpected = normalize(expectedOutput);

        // Validate line count first
        auto countNonEmptyLines = [](const std::string& str) {
            std::stringstream ss(str);
            std::string line;
            int count = 0;
            while (std::getline(ss, line)) {
                // Count lines that have content after trimming whitespace
                std::string trimmed = line;
                // Remove leading and trailing whitespace
                size_t first = trimmed.find_first_not_of(" \t\r\n");
                if (first == std::string::npos) {
                    continue; // Skip empty lines
                }
                size_t last = trimmed.find_last_not_of(" \t\r\n");
                trimmed = trimmed.substr(first, (last - first + 1));
                if (!trimmed.empty()) {
                    count++;
                }
            }
            return count;
        };

        int actualLines = countNonEmptyLines(normalizedOutput);
        int expectedLines = countNonEmptyLines(normalizedExpected);

        // If line counts don't match, log the discrepancy and fail
        if (actualLines != expectedLines) {
            std::cerr << "Line count mismatch in " << filePath << ":\n";
            std::cerr << "  Expected: " << expectedLines << " lines\n";
            std::cerr << "  Actual: " << actualLines << " lines\n";
            std::cerr << "  Difference: " << (actualLines - expectedLines) << "\n";
            return false;
        }

        // Then validate content
        return normalizedOutput == normalizedExpected;
    }
}
