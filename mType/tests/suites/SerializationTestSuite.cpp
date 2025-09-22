#include "SerializationTestSuite.hpp"
#include "../../services/ScriptInterpreter.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>

namespace tests::testSuite
{
    using namespace testFramework;
    using namespace services;

    void SerializationTestSuite::setupTests()
    {
        
        addOutputVerificationTest("Namespace Class Name Collisions",
                                  passPath + "namespaceClassNameCollisions.mt");

        addOutputVerificationTest("Switch Nested In Loops",
                                  passPath + "switchNestedInLoops.mt");

        addOutputVerificationTest("Test Import Main Compilation",
                                  passPath + "test_import_main.mt");
        

        addOutputVerificationTest("Class Generic Serialization",
                                  passPath + "classGenericSerialization.mt");

        addOutputVerificationTest("Collections as Data Members",
                                  passPath + "collectionsAsDataMembers.mt");

        addOutputVerificationTest("Multi-Type Generic Serialization",
                                  passPath + "multiTypeGenericSerialization.mt");

        addOutputVerificationTest("Generic Import Serialization",
                                  passPath + "genericImportSerialization.mt");
    }

    void SerializationTestSuite::run()
    {
        setupTests();

        std::cout << std::string(80, '=') << std::endl;
        std::cout << "Running " << suiteName << "..." << std::endl;
        std::cout << "Total Tests: " << testCases.size() << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        int passed = 0;
        int failed = 0;

        // Use a single ScriptInterpreter instance to test the real fix
        ScriptInterpreter interpreter;

        for (auto& testCase : testCases)
        {
            std::cout << "  Running: " << std::left << std::setw(60) << testCase.getName() << " ... ";

            try
            {

                std::string filePath = testCase.getFilePath();
                std::string mtcPath = filePath + "c";
                std::string expectedPath = filePath.substr(0, filePath.find_last_of('.')) + ".expected";

                // Clean up any existing .mtc file
                std::filesystem::remove(mtcPath);

                // Step 1: Compile .mt to .mtc
                bool compileSuccess = interpreter.compileScript(filePath, mtcPath);
                if (!compileSuccess)
                {
                    std::cout << "FAILED (compilation failed)" << std::endl;
                    failed++;
                    continue;
                }

                // Step 2: Run cached .mtc file
                std::stringstream outputBuffer;
                std::streambuf* oldCout = std::cout.rdbuf();
                std::cout.rdbuf(outputBuffer.rdbuf());

                bool runSuccess = interpreter.runCachedScript(mtcPath);

                std::cout.rdbuf(oldCout);
                std::string actualOutput = outputBuffer.str();

                // Trim trailing whitespace (newlines, spaces, etc.)
                while (!actualOutput.empty() && (actualOutput.back() == '\n' ||
                    actualOutput.back() == '\r' ||
                    actualOutput.back() == ' ' ||
                    actualOutput.back() == '\t'))
                {
                    actualOutput.pop_back();
                }

                if (!runSuccess)
                {
                    std::cout << "FAILED (cached execution failed)" << std::endl;
                    failed++;
                    continue;
                }

                // Step 3: Compare with expected output
                if (std::filesystem::exists(expectedPath))
                {
                    std::ifstream expectedFile(expectedPath);
                    std::string expectedOutput((std::istreambuf_iterator<char>(expectedFile)),
                                               std::istreambuf_iterator<char>());

                    // Trim trailing whitespace from expected output too
                    while (!expectedOutput.empty() && (expectedOutput.back() == '\n' ||
                        expectedOutput.back() == '\r' ||
                        expectedOutput.back() == ' ' ||
                        expectedOutput.back() == '\t'))
                    {
                        expectedOutput.pop_back();
                    }

                    // Normalize both strings by removing all whitespace and newlines for comparison
                    auto normalizeForComparison = [](const std::string& str) {
                        std::string result;
                        for (char c : str) {
                            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                                result += c;
                            }
                        }
                        return result;
                    };

                    std::string normalizedActual = normalizeForComparison(actualOutput);
                    std::string normalizedExpected = normalizeForComparison(expectedOutput);

                    if (normalizedActual == normalizedExpected)
                    {
                        std::cout << "PASSED" << std::endl;
                        passed++;
                    }
                    else
                    {
                        std::cout << "FAILED (output mismatch)" << std::endl;
                        std::cout << "    Expected length: " << expectedOutput.length() << std::endl;
                        std::cout << "    Actual length: " << actualOutput.length() << std::endl;
                        std::cout << "    Normalized expected length: " << normalizedExpected.length() << std::endl;
                        std::cout << "    Normalized actual length: " << normalizedActual.length() << std::endl;

                        // Show first difference in normalized strings
                        for (size_t i = 0; i < std::min(normalizedExpected.length(), normalizedActual.length()); ++i)
                        {
                            if (normalizedExpected[i] != normalizedActual[i])
                            {
                                std::cout << "    First normalized difference at position " << i << ": expected '"
                                    << normalizedExpected[i] << "' got '" << normalizedActual[i] << "'" << std::endl;
                                break;
                            }
                        }

                        // Show first difference in original strings (for debugging)
                        for (size_t i = 0; i < std::min(expectedOutput.length(), actualOutput.length()); ++i)
                        {
                            if (expectedOutput[i] != actualOutput[i])
                            {
                                std::cout << "    First raw difference at position " << i << ": expected '"
                                    << (int)expectedOutput[i] << "' got '" << (int)actualOutput[i] << "'" << std::endl;
                                break;
                            }
                        }
                        failed++;
                    }
                }
                else
                {
                    std::cout << "PASSED (no expected file)" << std::endl;
                    passed++;
                }

                // Clean up
                std::filesystem::remove(mtcPath);
            }
            catch (const std::exception& e)
            {
                std::cout << "FAILED (exception: " << e.what() << ")" << std::endl;
                failed++;
            }
        }

        // Print summary
        std::cout << std::string(80, '-') << std::endl;
        std::cout << "Suite Summary: " << suiteName << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        std::cout << "Total Tests: " << (passed + failed) << std::endl;
        std::cout << "Passed: " << passed << std::endl;
        std::cout << "Failed: " << failed << std::endl;
        std::cout << "Pass Rate: " << (100.0 * passed / (passed + failed)) << "%" << std::endl;

        if (failed == 0)
        {
            std::cout << "✓ Suite PASSED" << std::endl;
        }
        else
        {
            std::cout << "✗ Suite FAILED" << std::endl;
        }
        std::cout << std::string(80, '=') << std::endl;
    }
}
