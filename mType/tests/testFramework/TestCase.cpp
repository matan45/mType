#include "TestCase.hpp"
#include "../../parser/Parser.hpp"
#include "../../lexer/Lexer.hpp"
#include "../../services/ImportManager.hpp"
#include "../../evaluator/Evaluator.hpp"
#include "../../environment/EnvironmentBuilder.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../errors/TypeException.hpp"
#include "../../errors/ParseException.hpp"
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

            // Read file content
            std::ifstream file(filePath);
            if (!file.is_open())
            {
                status = TestStatus::ERROR;
                errorMessage = "Could not open test file: " + filePath;
                auto endTime = std::chrono::high_resolution_clock::now();
                executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                return;
            }

            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            file.close();

            // Redirect stdout to capture output
            std::stringstream outputBuffer;
            std::streambuf* oldCout = std::cout.rdbuf();
            std::cout.rdbuf(outputBuffer.rdbuf());

            try
            {
                // Initialize lexer and parser
                Lexer lexer(content);
                auto importManager = std::make_shared<ImportManager>();
                Parser parser(lexer);
                parser.setImportManager(importManager.get());

                // Parse the file
                auto ast = parser.parseProgram();

                if (!ast)
                {
                    throw std::runtime_error("Failed to generate AST");
                }

                // Evaluate the AST
                auto env = EnvironmentBuilder::createDefault();
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
        case TestType::PERFORMANCE:
            return "PERFORMANCE";
        default:
            return "UNKNOWN";
        }
    }
}
