#include "../tests/testFramework/TestSuite.hpp"
#include "../tests/suites/ControlFlowTestSuite.hpp"
#include "../tests/suites/ClassTestSuite.hpp"
#include "../tests/suites/ImportTestSuite.hpp"
#include "../tests/suites/IntegrationTestSuite.hpp"
#include "../tests/suites/TypeCheckingTestSuite.hpp"
#include "../tests/suites/ErrorTestSuite.hpp"
#include "../tests/suites/GenericsTestSuite.hpp"
#include "../tests/suites/ArrayTestSuite.hpp"
#include "../tests/suites/StringPoolTestSuite.hpp"
#include "../tests/suites/InterfaceTestSuite.hpp"
#include "../tests/suites/LambdaTestSuite.hpp"
#include "../tests/suites/CastTestSuite.hpp"
#include "../tests/suites/ModifiersTestSuite.hpp"
#include "../tests/suites/NativeTest.hpp"

#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../services/ScriptInterpreter.hpp"
#include "../vm/compiler/BytecodeCompiler.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../services/ImportManager.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../runtimeTypes/klass/MethodDefinition.hpp"
#include "../runtimeTypes/klass/ConstructorDefinition.hpp"

#include <vector>
#include <memory>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>


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
    else if (suiteName == "generics" || suiteName == "generic")
    {
        return std::make_unique<GenericsTestSuite>();
    }
    else if (suiteName == "arrays" || suiteName == "array")
    {
        return std::make_unique<ArrayTestSuite>();
    }
    else if (suiteName == "stringpool" || suiteName == "string-pool" || suiteName == "strings")
    {
        return std::make_unique<StringPoolTestSuite>();
    }
    else if (suiteName == "interface" || suiteName == "interfaces")
    {
        return std::make_unique<InterfaceTestSuite>();
    }
    else if (suiteName == "lambda" || suiteName == "lambdas")
    {
        return std::make_unique<LambdaTestSuite>();
    }
    else if (suiteName == "cast" || suiteName == "casting")
    {
        return std::make_unique<CastTestSuite>();
    }
    else if (suiteName == "modifiers" || suiteName == "modifier" || suiteName == "access")
    {
        return std::make_unique<ModifiersTestSuite>();
    }
    return nullptr;
}

void printAvailableTestSuites()
{
    std::cout << "Available test suites:\n";
    std::cout << "  control      - Control Flow Test Suite\n";
    std::cout << "  import       - Import Test Suite\n";
    std::cout << "  class        - Class Test Suite\n";
    std::cout << "  interface    - Interface Test Suite\n";
    std::cout << "  lambda       - Lambda Test Suite\n";
    std::cout << "  error        - Error Test Suite\n";
    std::cout << "  integration  - Integration Test Suite\n";
    std::cout << "  type         - Type Checking Test Suite\n";
    std::cout << "  generics     - Generics Test Suite\n";
    std::cout << "  arrays       - Array Test Suite\n";
    std::cout << "  string-pool  - String Pool Test Suite\n";
    std::cout << "  cast         - Cast and Type Checking Test Suite\n";
    std::cout << "  modifiers    - Access Modifiers Test Suite\n";
    std::cout << "  native       - Native C++ Integration Test Suite\n";
}

void runSpecificTestSuite(const std::string& suiteName,
                          constants::ExecutionMode execMode = constants::ExecutionMode::AST_INTERPRETER)
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

    auto suite = createTestSuite(suiteName);
    if (!suite)
    {
        std::cout << "Unknown test suite: " << suiteName << "\n\n";
        printAvailableTestSuites();
        return;
    }

    std::cout << "Running " << suite->getName() << "...\n\n";
    suite->setupTests();

    // Set execution mode on all test cases
    suite->setExecutionModeForAll(execMode);

    suite->run();
}

// Helper function to convert string type name to ValueType
// Note: stringToValueType and registerClassesFromMetadata have been refactored into ScriptInterpreter
// Note: resolveImports functionality has been refactored into ImportManager::resolveAllImports()

void runAllTests(constants::ExecutionMode execMode = constants::ExecutionMode::AST_INTERPRETER)
{
    std::cout << "Running all test suites...\n";
    std::cout << "Execution Mode: ";
    switch (execMode)
    {
    case constants::ExecutionMode::AST_INTERPRETER:
        std::cout << "AST Interpreter\n";
        break;
    case constants::ExecutionMode::BYTECODE_VM:
        std::cout << "Bytecode VM\n";
        break;
    case constants::ExecutionMode::DUAL_VALIDATION:
        std::cout << "Dual Validation\n";
        break;
    }
    std::cout << "\n";

    std::vector<std::unique_ptr<TestSuite>> suites;
    suites.push_back(std::make_unique<ControlFlowTestSuite>());
    suites.push_back(std::make_unique<ImportTestSuite>());
    suites.push_back(std::make_unique<ClassTestSuite>());
    suites.push_back(std::make_unique<ErrorTestSuite>());
    suites.push_back(std::make_unique<IntegrationTestSuite>());
    suites.push_back(std::make_unique<TypeCheckingTestSuite>());
    suites.push_back(std::make_unique<GenericsTestSuite>());
    suites.push_back(std::make_unique<ArrayTestSuite>());
    suites.push_back(std::make_unique<StringPoolTestSuite>());
    suites.push_back(std::make_unique<InterfaceTestSuite>());
    suites.push_back(std::make_unique<LambdaTestSuite>());
    suites.push_back(std::make_unique<CastTestSuite>());
    suites.push_back(std::make_unique<ModifiersTestSuite>());

    for (auto& suite : suites)
    {
        suite->setupTests(); // Initialize test cases
        suite->setExecutionModeForAll(execMode); // Set execution mode
        suite->run(); // Run tests and generate reports
    }

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
    // Parse execution mode first
    constants::ExecutionMode execMode = constants::ExecutionMode::AST_INTERPRETER;

    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--bytecode")
        {
            execMode = constants::ExecutionMode::BYTECODE_VM;
        }
        else if (std::string(argv[i]) == "--dual")
        {
            execMode = constants::ExecutionMode::DUAL_VALIDATION;
        }
    }

    // Check for test suite execution
    if (argc >= 2 && std::string(argv[argc - 2]) == "--test" && argc >= 3)
    {
        std::string suiteName = argv[argc - 1];
        runSpecificTestSuite(suiteName, execMode);
        return 0;
    }

    if (argc >= 2 && std::string(argv[argc - 1]) == "--tests")
    {
        runAllTests(execMode);
        return 0;
    }

    if (argc == 2 && std::string(argv[1]) == "--tests")
    {
        runAllTests(execMode);
        return 0;
    }

    if (argc == 3 && std::string(argv[1]) == "--test")
    {
        std::string suiteName = argv[2];
        runSpecificTestSuite(suiteName, execMode);

        return 0;
    }

    if (argc >= 2 && std::string(argv[1]) == "--help")
    {
        std::cout << "Usage:\n";
        std::cout << "  " << argv[0] << " <script_file.mt>           - Run a script file (AST interpreter mode)\n";
        std::cout << "  " << argv[0] << " --bytecode <script.mt>     - Run with bytecode VM\n";
        std::cout << "  " << argv[0] << " --dual <script.mt>         - Run with dual validation (AST + Bytecode)\n";
        std::cout << "  " << argv[0] << " -O<level> <script.mt>      - Set optimization level (0-3)\n";
        std::cout << "  " << argv[0] << " --compile <script.mt>      - Compile to bytecode file (.mtc)\n";
        std::cout << "  " << argv[0] << " --run-cached <file.mtc>    - Run pre-compiled bytecode file\n";
        std::cout << "  " << argv[0] << " --tests                    - Run all test suites\n";
        std::cout << "  " << argv[0] << " --bytecode --tests         - Run all test suites in bytecode mode\n";
        std::cout << "  " << argv[0] << " --test <suite>             - Run specific test suite\n";
        std::cout << "  " << argv[0] << " --bytecode --test <suite>  - Run test suite in bytecode mode\n";
        std::cout << "  " << argv[0] << " --help                     - Show this help message\n\n";
        std::cout << "Execution Modes:\n";
        std::cout << "  AST Interpreter (default) - Traditional AST walking interpreter\n";
        std::cout << "  Bytecode VM (--bytecode)  - Stack-based bytecode virtual machine\n";
        std::cout << "  Dual Validation (--dual)  - Run both and compare results\n\n";
        std::cout << "Optimization Levels:\n";
        std::cout << "  -O0 - No optimization (default)\n";
        std::cout << "  -O1 - Basic optimization\n";
        std::cout << "  -O2 - Advanced optimization\n";
        printAvailableTestSuites();
        return 0;
    }

    // Handle --compile command
    if (argc == 3 && std::string(argv[1]) == "--compile")
    {
        std::string sourceFile = argv[2];
        std::string outputFile = sourceFile + "c"; // .mt -> .mtc

        try
        {
            std::cout << "Compiling " << sourceFile << " to " << outputFile << "...\n";

            ScriptInterpreter interpreter;
            interpreter.compileToFile(sourceFile, outputFile);
            return 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Compilation error: " << e.what() << std::endl;
            return 1;
        }
    }

    // Handle --run-cached command
    if (argc == 3 && std::string(argv[1]) == "--run-cached")
    {
        std::string cachedFile = argv[2];

        try
        {
            std::cout << "Loading cached bytecode from " << cachedFile << "...\n";

            ScriptInterpreter interpreter;
            interpreter.runCompiledBytecode(cachedFile);
            return 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Execution error: " << e.what() << std::endl;
            return 1;
        }
    }

    // Parse optimization level and filename
    constants::OptimizationLevel optLevel = constants::OptimizationLevel::O0;
    std::string filename;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--bytecode")
        {
            execMode = constants::ExecutionMode::BYTECODE_VM;
        }
        else if (arg == "--dual")
        {
            execMode = constants::ExecutionMode::DUAL_VALIDATION;
        }
        else if (arg.substr(0, 2) == "-O" && arg.length() == 3)
        {
            char level = arg[2];
            if (level == '0') optLevel = constants::OptimizationLevel::O0;
            else if (level == '1') optLevel = constants::OptimizationLevel::O1;
            else if (level == '2') optLevel = constants::OptimizationLevel::O2;
            else
            {
                std::cerr << "Invalid optimization level: " << arg << std::endl;
                return 1;
            }
        }
        else if (arg[0] != '-')
        {
            filename = arg;
        }
    }

    if (filename.empty())
    {
        std::cerr << "Error: No script file specified\n";
        std::cerr << "Use --help for usage information\n";
        return 1;
    }

    try
    {
        ScriptInterpreter interpreter(execMode, optLevel);

        // Print execution mode
        std::cout << "Execution Mode: ";
        switch (execMode)
        {
        case constants::ExecutionMode::AST_INTERPRETER:
            std::cout << "AST Interpreter";
            break;
        case constants::ExecutionMode::BYTECODE_VM:
            std::cout << "Bytecode VM";
            break;
        case constants::ExecutionMode::DUAL_VALIDATION:
            std::cout << "Dual Validation";
            break;
        }
        std::cout << " (Optimization Level: O" << static_cast<int>(optLevel) << ")\n\n";

        interpreter.runScript(filename);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
