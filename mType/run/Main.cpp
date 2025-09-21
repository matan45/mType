#include "../tests/testFramework/TestSuite.hpp"
#include "../tests/suites/ControlFlowTestSuite.hpp"
#include "../tests/suites/ClassTestSuite.hpp"
#include "../tests/suites/ImportTestSuite.hpp"
#include "../tests/suites/IntegrationTestSuite.hpp"
#include "../tests/suites/TypeCheckingTestSuite.hpp"
#include "../tests/suites/ErrorTestSuite.hpp"
#include "../tests/suites/CollectionsTestSuite.hpp"
#include "../tests/suites/GenericsTestSuite.hpp"
#include "../tests/suites/ArrayTestSuite.hpp"
#include "../tests/suites/SerializationTestSuite.hpp"
#include "../tests/suites/NativeTest.hpp"

#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../services/ScriptInterpreter.hpp"

#include <vector>
#include <memory>
#include <iostream>
#include <string>
#include <filesystem>


using namespace tests::testSuite;
using namespace tests::testFramework;
using namespace parser;
using namespace lexer;
using namespace services;
using namespace evaluator;
using namespace environment;


std::unique_ptr<TestSuite> createTestSuite(const std::string& suiteName)
{
    if (suiteName == "control" || suiteName == "controlflow")
    {
        return std::make_unique<ControlFlowTestSuite>();
    }
    else if (suiteName == "import" || suiteName == "imports")
    {
        return std::make_unique<ImportTestSuite>();
    }
    else if (suiteName == "class" || suiteName == "classes")
    {
        return std::make_unique<ClassTestSuite>();
    }
    else if (suiteName == "error" || suiteName == "errors")
    {
        return std::make_unique<ErrorTestSuite>();
    }
    else if (suiteName == "integration")
    {
        return std::make_unique<IntegrationTestSuite>();
    }
    else if (suiteName == "type" || suiteName == "typechecking")
    {
        return std::make_unique<TypeCheckingTestSuite>();
    }
    else if (suiteName == "collections" || suiteName == "collection")
    {
        return std::make_unique<CollectionsTestSuite>();
    }
    else if (suiteName == "generics" || suiteName == "generic")
    {
        return std::make_unique<GenericsTestSuite>();
    }
    else if (suiteName == "arrays" || suiteName == "array")
    {
        return std::make_unique<ArrayTestSuite>();
    }
    else if (suiteName == "serialization" || suiteName == "serialize" || suiteName == "compile")
    {
        return std::make_unique<SerializationTestSuite>();
    }
    return nullptr;
}

void printAvailableTestSuites()
{
    std::cout << "Available test suites:\n";
    std::cout << "  control      - Control Flow Test Suite\n";
    std::cout << "  import       - Import Test Suite\n";
    std::cout << "  class        - Class Test Suite\n";
    std::cout << "  error        - Error Test Suite\n";
    std::cout << "  integration  - Integration Test Suite\n";
    std::cout << "  type         - Type Checking Test Suite\n";
    std::cout << "  collections  - Collections Test Suite\n";
    std::cout << "  generics     - Generics Test Suite\n";
    std::cout << "  arrays       - Array Test Suite\n";
    std::cout << "  serialization- Serialization & Compilation Test Suite\n";
    std::cout << "  native       - Native C++ Integration Test Suite\n";
}

void runSpecificTestSuite(const std::string& suiteName)
{
    // Handle native test separately since it doesn't inherit from TestSuite
    if (suiteName == "native")
    {
        std::cout << "Running Native C++ Integration Test Suite...\n\n";
        auto nativeTest = std::make_unique<NativeTest>();
        nativeTest->setupTests();
        nativeTest->runCustomTests();
        return;
    }

    // Handle serialization test separately since it needs custom execution
    if (suiteName == "serialization" || suiteName == "serialize" || suiteName == "compile")
    {
        std::cout << "Running Serialization Test Suite...\n\n";
        auto serializationTest = std::make_unique<SerializationTestSuite>();
        serializationTest->run();
        return;
    }

    auto suite = createTestSuite(suiteName);
    if (!suite)
    {
        std::cout << "Unknown test suite: " << suiteName << "\n\n";
        printAvailableTestSuites();
        return;
    }

    std::cout << "Running " << suite->getName() << "...\n\n";
    suite->setupTests();


    suite->run();
}

void runAllTests()
{
    std::cout << "Running all test suites...\n\n";

    std::vector<std::unique_ptr<TestSuite>> suites;
    suites.push_back(std::make_unique<ControlFlowTestSuite>());
    suites.push_back(std::make_unique<ImportTestSuite>());
    suites.push_back(std::make_unique<ClassTestSuite>());
    suites.push_back(std::make_unique<ErrorTestSuite>());
    suites.push_back(std::make_unique<IntegrationTestSuite>());
    suites.push_back(std::make_unique<TypeCheckingTestSuite>());
    suites.push_back(std::make_unique<GenericsTestSuite>());
    suites.push_back(std::make_unique<ArrayTestSuite>());
    suites.push_back(std::make_unique<CollectionsTestSuite>());

    for (auto& suite : suites)
    {
        suite->setupTests(); // Initialize test cases
        suite->run(); // Run tests and generate reports
    }

    // Run serialization tests separately
    std::cout << "\nRunning Serialization Test Suite...\n";
    auto serializationTest = std::make_unique<SerializationTestSuite>();
    serializationTest->run();

    // Run native tests separately
    std::cout << "\nRunning Native C++ Integration Test Suite...\n";
    auto nativeTest = std::make_unique<NativeTest>();
    nativeTest->setupTests();
    nativeTest->runCustomTests();

    // Print final summary
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "ALL TEST SUITES COMPLETED" << std::endl;
    std::cout << "Reports generated in test_reports/ directory" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
}


int main(int argc, char* argv[])
{
    if (argc == 2 && std::string(argv[1]) == "--tests")
    {
        runAllTests();
        return 0;
    }

    if (argc == 3 && std::string(argv[1]) == "--test")
    {
        std::string suiteName = argv[2];
        runSpecificTestSuite(suiteName);
        return 0;
    }

    // Handle compile-only mode
    if (argc >= 3 && std::string(argv[1]) == "--compile")
    {
        std::string sourceFile = argv[2];
        std::string outputFile = "";

        if (argc >= 4)
        {
            outputFile = argv[3];
        }

        try
        {
            ScriptInterpreter interpreter;
            if (interpreter.compileScript(sourceFile, outputFile))
            {
                std::cout << "Successfully compiled " << sourceFile;
                if (!outputFile.empty())
                {
                    std::cout << " to " << outputFile;
                }
                else
                {
                    std::cout << " to " << sourceFile << "c";
                }
                std::cout << std::endl;
                return 0;
            }
            else
            {
                std::cerr << "Failed to compile " << sourceFile << std::endl;
                return 1;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

    // Handle run-cached mode
    if (argc == 3 && std::string(argv[1]) == "--run-cached")
    {
        std::string cachedFile = argv[2];

        try
        {
            ScriptInterpreter interpreter;
            if (!interpreter.runCachedScript(cachedFile))
            {
                return 1;
            }
            return 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

    if (argc == 2 && std::string(argv[1]) == "--help")
    {
        std::cout << "Usage:\n";
        std::cout << "  " << argv[0] << " <script_file.mt>           - Run a script file (with auto-caching)\n";
        std::cout << "  " << argv[0] << " --compile <file.mt> [out]  - Compile script to AST cache\n";
        std::cout << "  " << argv[0] << " --run-cached <file.mtc>    - Run pre-compiled AST cache\n";
        std::cout << "  " << argv[0] << " --tests                    - Run all test suites\n";
        std::cout << "  " << argv[0] << " --test <suite>             - Run specific test suite\n";
        std::cout << "  " << argv[0] << " --help                     - Show this help message\n\n";
        printAvailableTestSuites();
        return 0;
    }

    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <script_file.mt> or --tests or --test <suite>" << std::endl;
        std::cout << "       " << argv[0] << " --compile <file.mt> [output] or --run-cached <file.mtc>" << std::endl;
        std::cout << "Use --help for detailed usage information" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    try
    {
        ScriptInterpreter interpreter;
        interpreter.runScript(filename);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
