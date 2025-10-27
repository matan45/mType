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
#include "../tests/suites/AwaitTestSuite.hpp"
#include "../tests/suites/AnnotationTestSuite.hpp"

#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../services/ScriptInterpreter.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"

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
    else if (suiteName == "await" || suiteName == "async")
    {
        return std::make_unique<AwaitTestSuite>();
    }
    else if (suiteName == "annotation" || suiteName == "annotations")
    {
        return std::make_unique<AnnotationTestSuite>();
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
    std::cout << "  await        - Async/Await Test Suite\n";
    std::cout << "  annotation   - Annotation Test Suite\n";
    std::cout << "  native       - Native C++ Integration Test Suite\n";
}

void runSpecificTestSuite(const std::string& suiteName,
                          constants::ExecutionMode execMode = constants::ExecutionMode::AST_INTERPRETER)
{
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

/**
 * Find and print all classes with @Script annotation
 * This demonstrates the use case for @Script annotation
 */
void printScriptAnnotatedClasses(std::shared_ptr<environment::Environment> environment)
{
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "Classes with @Script annotation:\n";
    std::cout << std::string(80, '=') << "\n\n";

    auto classRegistry = environment->getClassRegistry();
    if (!classRegistry)
    {
        std::cout << "No class registry available\n";
        return;
    }

    auto allClassNames = classRegistry->getAllItemNames();
    int scriptClassCount = 0;

    for (const auto& className : allClassNames)
    {
        auto classDef = classRegistry->findClass(className);
        if (classDef && classDef->hasAnnotation("Script"))
        {
            scriptClassCount++;

            std::cout << "Class: " << className << "\n";

            // Print instance methods
            const auto& methods = classDef->getInstanceMethods();
            if (!methods.empty())
            {
                std::cout << "  Instance Methods:\n";
                for (const auto& [methodName, methodDef] : methods)
                {
                    std::cout << "    - " << methodName << "()\n";
                }
            }

            // Print static methods
            const auto& staticMethods = classDef->getStaticMethods();
            if (!staticMethods.empty())
            {
                std::cout << "  Static Methods:\n";
                for (const auto& [methodName, methodDef] : staticMethods)
                {
                    std::cout << "    - static " << methodName << "()\n";
                }
            }

            // Print instance fields
            const auto& fields = classDef->getInstanceFields();
            if (!fields.empty())
            {
                std::cout << "  Instance Fields:\n";
                for (const auto& [fieldName, fieldDef] : fields)
                {
                    std::cout << "    - " << fieldName << "\n";
                }
            }

            // Print static fields
            const auto& staticFields = classDef->getStaticFields();
            if (!staticFields.empty())
            {
                std::cout << "  Static Fields:\n";
                for (const auto& [fieldName, fieldDef] : staticFields)
                {
                    std::cout << "    - static " << fieldName << "\n";
                }
            }

            std::cout << "\n";
        }
    }

    std::cout << std::string(80, '-') << "\n";
    std::cout << "Total @Script annotated classes: " << scriptClassCount << "\n";
    std::cout << std::string(80, '=') << "\n";
}

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
    suites.push_back(std::make_unique<AwaitTestSuite>());
    suites.push_back(std::make_unique<AnnotationTestSuite>());

    for (auto& suite : suites)
    {
        suite->setupTests(); // Initialize test cases
        suite->setExecutionModeForAll(execMode); // Set execution mode
        suite->run(); // Run tests and generate reports
    }

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
        std::cout << "  " << argv[0] << " -debug <script.mt>         - Run with debug mode (no optimization)\n";
        std::cout << "  " << argv[0] << " -release <script.mt>       - Run with release mode (full optimization)\n";
        std::cout << "  " << argv[0] << " --compile <script.mt>      - Compile to bytecode file (.mtc)\n";
        std::cout << "  " << argv[0] << " --compile -release <script.mt> - Compile with optimizations\n";
        std::cout << "  " << argv[0] << " --run-cached <file.mtc>    - Run pre-compiled bytecode file\n";
        std::cout << "  " << argv[0] << " --find-script-classes <script.mt> - Run script and show all @Script classes\n";
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
        std::cout << "  -debug   - Debug mode (no dead code optimization)\n";
        std::cout << "  -release - Release mode (includes dead code elimination and unused declaration removal)\n";
        printAvailableTestSuites();
        return 0;
    }

    // Handle --compile command (supports --compile -release <file.mt>)
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--compile")
        {
            std::string sourceFile;
            constants::OptimizationLevel compileOptLevel = constants::OptimizationLevel::Debug;

            // Parse remaining arguments for optimization level and filename
            for (int j = i + 1; j < argc; ++j)
            {
                std::string arg = argv[j];
                if (arg == "-release")
                {
                    compileOptLevel = constants::OptimizationLevel::Release;
                }
                else if (arg == "-debug")
                {
                    compileOptLevel = constants::OptimizationLevel::Debug;
                }
                else if (arg[0] != '-')
                {
                    sourceFile = arg;
                }
            }

            if (sourceFile.empty())
            {
                std::cerr << "Error: No source file specified for --compile\n";
                return 1;
            }

            std::string outputFile = sourceFile + "c"; // .mt -> .mtc

            try
            {
                std::cout << "Compiling " << sourceFile << " to " << outputFile;
                std::cout << " (Optimization: " << (compileOptLevel == constants::OptimizationLevel::Release
                                                        ? "Release"
                                                        : "Debug") << ")...\n";

                ScriptInterpreter interpreter(constants::ExecutionMode::BYTECODE_VM, compileOptLevel);
                interpreter.compileToFile(sourceFile, outputFile);
                return 0;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Compilation error: " << e.what() << std::endl;
                return 1;
            }
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

    // Handle --find-script-classes command
    if (argc >= 3 && std::string(argv[1]) == "--find-script-classes")
    {
        std::string scriptFile = argv[2];

        // Check for optional execution mode flags
        constants::ExecutionMode mode = constants::ExecutionMode::BYTECODE_VM; // Use bytecode for analysis
        for (int i = 3; i < argc; ++i)
        {
            if (std::string(argv[i]) == "--bytecode")
            {
                mode = constants::ExecutionMode::BYTECODE_VM;
            }
        }

        try
        {
            std::cout << "Analyzing script: " << scriptFile << "\n";
            std::cout << "(Classes are registered but script is not executed)\n\n";

            ScriptInterpreter interpreter(mode);

            // Parse and register classes without executing the script
            interpreter.parseAndRegisterClasses(scriptFile);

            // Query and print all @Script annotated classes
            auto environment = interpreter.getEnvironment();
            printScriptAnnotatedClasses(environment);

            return 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

    // Parse optimization level and filename
    constants::OptimizationLevel optLevel = constants::OptimizationLevel::Debug;
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
        else if (arg == "-debug")
        {
            optLevel = constants::OptimizationLevel::Debug;
        }
        else if (arg == "-release")
        {
            optLevel = constants::OptimizationLevel::Release;
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
        std::cout << " (Optimization: ";
        switch (optLevel)
        {
        case constants::OptimizationLevel::Debug:
            std::cout << "Debug";
            break;
        case constants::OptimizationLevel::Release:
            std::cout << "Release";
            break;
        }
        std::cout << ")\n\n";

        interpreter.runScript(filename);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
