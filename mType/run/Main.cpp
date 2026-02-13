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
#include "../tests/suites/OverloadingTestSuite.hpp"
#include "../tests/suites/IteratorTestSuite.hpp"
#include "../tests/suites/EnhancedForLoopTestSuite.hpp"
#include "../tests/suites/StreamTestSuite.hpp"
#include "../tests/suites/CollectionsTestSuite.hpp"
#include "../tests/suites/ReflectionTestSuite.hpp"
#include "../tests/suites/GCTestSuite.hpp"

#include "../gc/GC.hpp"
#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../services/ScriptInterpreter.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../debugger/DebugContext.hpp"
#include "../debugger/DebugHookHelper.hpp"
#include "../reflection/ReflectionNatives.hpp"
#include "../debugger/DebugProtocol.hpp"
#include "../project/ProjectConfigParser.hpp"
#include "../project/ProjectBuilder.hpp"

#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <thread>


using namespace tests::testSuite;
using namespace tests::testFramework;
using namespace parser;
using namespace lexer;
using namespace services;
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
    else if (suiteName == "overload" || suiteName == "overloading")
    {
        return std::make_unique<OverloadingTestSuite>();
    }
    else if (suiteName == "iterator" || suiteName == "iterators")
    {
        return std::make_unique<IteratorTestSuite>();
    }
    else if (suiteName == "foreach" || suiteName == "enhancedfor" || suiteName == "for-each")
    {
        return std::make_unique<EnhancedForLoopTestSuite>();
    }
    else if (suiteName == "stream" || suiteName == "streams")
    {
        return std::make_unique<StreamTestSuite>();
    }
    else if (suiteName == "collections" || suiteName == "collection")
    {
        return std::make_unique<CollectionsTestSuite>();
    }
    else if (suiteName == "reflection" || suiteName == "reflect")
    {
        return std::make_unique<ReflectionTestSuite>();
    }
    else if (suiteName == "gc" || suiteName == "garbage" || suiteName == "garbage-collection")
    {
        return std::make_unique<GCTestSuite>();
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
    std::cout << "  overloading  - Method/Function Overloading Test Suite\n";
    std::cout << "  iterator     - Iterator Protocol Test Suite\n";
    std::cout << "  foreach      - Enhanced For-Loop Test Suite\n";
    std::cout << "  stream       - Stream API Test Suite\n";
    std::cout << "  collections  - Collections (ArrayList, LinkedList, HashMap) Test Suite\n";
    std::cout << "  reflection   - Reflection API Test Suite\n";
    std::cout << "  gc           - Garbage Collection Test Suite\n";
    std::cout << "  native       - Native C++ Integration Test Suite\n";
}

void runSpecificTestSuite(const std::string& suiteName,
                          constants::ExecutionMode execMode = constants::ExecutionMode::BYTECODE_VM)
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

    // Clean up GC to avoid static destruction order issues
    gc::GC::shutdown();

    // Cleanup reflection static state to avoid static destruction order issues
    reflection::ReflectionNatives::cleanup();
}

// Helper function to convert string type name to ValueType
// Note: stringToValueType and registerClassesFromMetadata have been refactored into ScriptInterpreter
// Note: resolveImports functionality has been refactored into ImportManager::resolveAllImports()

/**
 * Demonstrate creating objects and calling methods on @Script classes
 */
void demonstrateScriptObjectUsage(const std::string& scriptFile,
                                  constants::ExecutionMode execMode = constants::ExecutionMode::BYTECODE_VM)
{
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "Script Class Object Usage Demo\n";
    std::cout << "Execution Mode: Bytecode VM\n";
    std::cout << std::string(80, '=') << "\n\n";

    try
    {
        // Step 1: Parse and register classes
        std::cout << "Step 1: Parsing script and registering classes...\n";
        ScriptInterpreter interpreter(execMode);
        interpreter.parseAndRegisterClasses(scriptFile);
        std::cout << "  Done!\n\n";

        // Step 2: Find @Script classes
        std::cout << "Step 2: Finding @Script annotated classes...\n";
        auto environment = interpreter.getEnvironment();
        auto classRegistry = environment->getClassRegistry();
        auto allClassNames = classRegistry->getAllItemNames();

        std::vector<std::string> scriptClasses;
        for (const auto& className : allClassNames)
        {
            auto classDef = classRegistry->findClass(className);
            if (classDef && classDef->hasAnnotation("Script"))
            {
                scriptClasses.push_back(className);
                std::cout << "  Found: " << className << "\n";
            }
        }
        std::cout << "\n";

        // Step 3: Create and use PlayerController
        if (std::find(scriptClasses.begin(), scriptClasses.end(), "PlayerController") != scriptClasses.end())
        {
            std::cout << "Step 3: Creating PlayerController instance...\n";

            std::vector<value::Value> ctorArgs;
            ctorArgs.push_back(value::Value(100)); // health = 100

            value::Value player = interpreter.createObject("PlayerController", ctorArgs);
            std::cout << "  PlayerController created with health=100\n\n";

            std::cout << "Step 4: Calling methods on PlayerController...\n";

            // Call getHealth()
            std::vector<value::Value> noArgs;
            value::Value health = interpreter.callMethod(player, "getHealth", noArgs);
            std::cout << "  player.getHealth() = " << std::get<int64_t>(health) << "\n";

            // Call takeDamage(30)
            std::vector<value::Value> damageArgs;
            damageArgs.push_back(value::Value(30));
            interpreter.callMethod(player, "takeDamage", damageArgs);
            std::cout << "  player.takeDamage(30) called\n";

            // Call getHealth() again
            value::Value newHealth = interpreter.callMethod(player, "getHealth", noArgs);
            std::cout << "  player.getHealth() = " << std::get<int64_t>(newHealth) << " (after damage)\n\n";
        }

        // Step 5: Create and use GameWorld
        if (std::find(scriptClasses.begin(), scriptClasses.end(), "GameWorld") != scriptClasses.end())
        {
            std::cout << "Step 5: Creating GameWorld instance...\n";

            std::vector<value::Value> worldArgs;
            worldArgs.push_back(value::Value(5)); // level = 5

            value::Value world = interpreter.createObject("GameWorld", worldArgs);
            std::cout << "  GameWorld created with level=5\n";

            // Call getLevel()
            std::vector<value::Value> noArgs;
            value::Value level = interpreter.callMethod(world, "getLevel", noArgs);
            std::cout << "  world.getLevel() = " << std::get<int64_t>(level) << "\n\n";
        }

        std::cout << std::string(80, '=') << "\n";
        std::cout << "Demo Complete!\n";
        std::cout << std::string(80, '=') << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

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

void runAllTests(constants::ExecutionMode execMode = constants::ExecutionMode::BYTECODE_VM)
{
    std::cout << "Running all test suites...\n";
    std::cout << "Execution Mode: Bytecode VM\n\n";

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
    suites.push_back(std::make_unique<OverloadingTestSuite>());
    suites.push_back(std::make_unique<IteratorTestSuite>());
    suites.push_back(std::make_unique<EnhancedForLoopTestSuite>());
    suites.push_back(std::make_unique<StreamTestSuite>());
    suites.push_back(std::make_unique<ReflectionTestSuite>());
    suites.push_back(std::make_unique<GCTestSuite>());

    for (auto& suite : suites)
    {
        suite->setupTests(); // Initialize test cases
        suite->setExecutionModeForAll(execMode); // Set execution mode
        suite->run(); // Run tests and generate reports
    }

    // Cleanup reflection static state to avoid static destruction order issues
    reflection::ReflectionNatives::cleanup();

    // Print final summary
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "ALL TEST SUITES COMPLETED" << std::endl;
    std::cout << "Reports generated in test_reports/ directory" << std::endl;

    std::cout << std::string(80, '=') << std::endl;
}

/**
 * Run script in debug mode with debugger protocol active
 */
void runInDebugMode(const std::string& filename,
                    constants::ExecutionMode execMode = constants::ExecutionMode::BYTECODE_VM)
{
    // Output debug banner to stderr so it doesn't interfere with debug protocol on stdout
    std::cerr << "\n" << std::string(80, '=') << "\n";
    std::cerr << "mType Debugger Mode\n";
    std::cerr << std::string(80, '=') << "\n";
    std::cerr << "Debug protocol: stdin/stdout\n";
    std::cerr << "Script file: " << filename << "\n";
    std::cerr << std::string(80, '=') << "\n\n";

    try
    {
        // Initialize debug context
        debugger::DebugContext::initialize();
        auto& debugCtx = debugger::DebugContext::getInstance();

        // Create interpreter and enable debugging
        ScriptInterpreter interpreter(execMode);
        interpreter.enableDebugging();

        // Start debug server in a separate thread
        debugger::DebugServer debugServer;

        // Set the environment for variable inspection
        debugServer.setEnvironment(interpreter.getEnvironment());

        std::thread serverThread([&debugServer]()
        {
            debugServer.run();
        });

        // Notify debugger that script is starting
        debugger::DebugHookHelper::notifyScriptStart(filename);

        // Push main script frame before pausing so VSCode has a frame to show
        errors::SourceLocation mainFrameLoc(filename, 1, 1);
        debugger::DebugHookHelper::enterFunctionHook("<main>", mainFrameLoc);

        // Pause at entry to allow debugger to set breakpoints
        debugCtx.pause();
        std::cerr << "Paused at entry. Waiting for debugger commands...\n";

        // Send STOPPED event with reason "entry" to notify VSCode
        errors::SourceLocation entryLocation(filename, 1, 1);
        debugger::DebugProtocol::sendStoppedEvent("entry", entryLocation);

        // Wait for debugger to set breakpoints and send CONTINUE
        debugCtx.waitForResume();

        // In bytecode mode, update DebugServer to use VM for variable inspection
        auto vm = interpreter.getVM();
        if (vm)
        {
            debugServer.setVM(vm);
        }

        // Run the script (will pause at breakpoints)
        interpreter.runScript(filename);

        // Pop main script frame after completion
        debugger::DebugHookHelper::exitFunctionHook("<main>");

        // Notify debugger that script completed
        debugger::DebugHookHelper::notifyScriptComplete(filename);

        // Stop debug server
        debugServer.stop();
        if (serverThread.joinable())
        {
            serverThread.join();
        }

        // Shutdown debug context
        debugger::DebugContext::shutdown();

        std::cerr << "\n" << std::string(80, '=') << "\n";
        std::cerr << "Debug session ended\n";
        std::cerr << std::string(80, '=') << "\n";
    }
    catch (const std::exception& e)
    {
        // Pop main script frame on exception
        if (debugger::DebugHookHelper::isDebuggingEnabled())
        {
            debugger::DebugHookHelper::exitFunctionHook("<main>");
        }
        std::cerr << "Debug session error: " << e.what() << std::endl;
        debugger::DebugContext::shutdown();
    }
}


int main(int argc, char* argv[])
{
    // Bytecode VM is the only execution mode
    constants::ExecutionMode execMode = constants::ExecutionMode::BYTECODE_VM;

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
        std::cout << "  " << argv[0] << " <script_file.mt>           - Run a script file\n";
        std::cout << "  " << argv[0] << " --debug <script.mt>        - Run with debugger (breakpoints, stepping)\n";
        std::cout << "  " << argv[0] << " --gc-stats <script.mt>     - Run and print GC statistics after execution\n";
        std::cout << "  " << argv[0] << " --no-jit <script.mt>       - Disable JIT compilation (JIT is on by default)\n";
        std::cout << "  " << argv[0] << " --compile <script.mt>      - Compile to bytecode file (.mtc)\n";
        std::cout << "  " << argv[0] << " --run-cached <file.mtc>    - Run pre-compiled bytecode file\n";
        std::cout << "  " << argv[0] << " --build [project.mtproj]   - Build project (compile all files to bytecode)\n";
        std::cout << "  " << argv[0] << " --build --lib [.mtproj]    - Build project into single .mtcLib file\n";
        std::cout << "  " << argv[0] << " --clean [project.mtproj]   - Remove compiled bytecode files\n";
        std::cout << "  " << argv[0] <<
            " --init <name> <include>    - Create new .mtproj file (e.g. --init MyApp src/**/*.mt)\n";
        std::cout << "  " << argv[0] << " --add <pattern> [.mtproj]  - Add include pattern to project\n";
        std::cout << "  " << argv[0] << " --remove <pattern> [.mtproj] - Remove include pattern from project\n";
        std::cout << "  " << argv[0] <<
            " --find-script-classes <script.mt> - Analyze script and show all @Script classes\n";
        std::cout << "  " << argv[0] <<
            " --test-script-objects <script.mt> - Demo: Create objects and call methods from C++\n";
        std::cout << "  " << argv[0] << " --tests                    - Run all test suites\n";
        std::cout << "  " << argv[0] << " --test <suite>             - Run specific test suite\n";
        std::cout << "  " << argv[0] << " --help                     - Show this help message\n\n";
        printAvailableTestSuites();
        return 0;
    }

    // Handle --build command (project compilation)
    if (argc >= 2 && std::string(argv[1]) == "--build")
    {
        std::string mtprojPath;
        bool buildLib = false;

        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--lib")
            {
                buildLib = true;
            }
            else if (arg[0] != '-')
            {
                mtprojPath = arg;
            }
        }

        try
        {
            project::ProjectConfigParser parser;

            if (mtprojPath.empty())
            {
                auto found = parser.findProject(".");
                if (!found)
                {
                    std::cerr << "Error: No .mtproj file found in current directory or parents\n";
                    return 1;
                }
                mtprojPath = *found;
            }

            std::cout << "Loading project: " << mtprojPath << "\n";

            auto config = parser.parse(mtprojPath);

            std::cout << "Project: " << config->name;
            if (!config->version.empty())
            {
                std::cout << " v" << config->version;
            }
            std::cout << "\n";
            std::cout << "Source files: " << config->resolvedSourceFiles.size() << "\n";
            std::cout << "Output directory: " << config->output.directory << "\n";

            project::ProjectBuilder builder;

            builder.setProgressCallback([](const project::BuildProgress& progress)
            {
                std::cout << "[" << progress.current << "/" << progress.total << "] " << progress.currentFile << "\n";
            });

            project::BuildResult result;

            if (buildLib)
            {
                // Build into single library file
                std::filesystem::path outputDir = std::filesystem::path(config->projectRoot) / config->output.directory;
                std::string libPath = (outputDir / (config->name + ".mtcLib")).string();
                std::cout << "Building library: " << libPath << "\n\n";
                result = builder.buildLibrary(*config, libPath);
            }
            else
            {
                std::cout << "\n";
                result = builder.build(*config);
            }

            std::cout << "\nBuild " << (result.success ? "succeeded" : "failed") << "\n";
            std::cout << "  Compiled: " << result.filesCompiled << " files\n";
            if (result.filesFailed > 0)
            {
                std::cout << "  Failed:   " << result.filesFailed << " files\n";
            }
            std::cout << "  Duration: " << result.duration.count() << "ms\n";

            if (!result.errors.empty())
            {
                std::cout << "\nErrors:\n";
                for (const auto& error : result.errors)
                {
                    std::cout << "  " << error << "\n";
                }
            }

            return result.success ? 0 : 1;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Build error: " << e.what() << std::endl;
            return 1;
        }
    }

    // Handle --clean command
    if (argc >= 2 && std::string(argv[1]) == "--clean")
    {
        std::string mtprojPath;

        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg[0] != '-')
            {
                mtprojPath = arg;
                break;
            }
        }

        try
        {
            project::ProjectConfigParser parser;

            if (mtprojPath.empty())
            {
                auto found = parser.findProject(".");
                if (!found)
                {
                    std::cerr << "Error: No .mtproj file found in current directory or parents\n";
                    return 1;
                }
                mtprojPath = *found;
            }

            auto config = parser.parse(mtprojPath);

            project::ProjectBuilder builder;
            builder.clean(*config);

            std::cout << "Clean completed. Removed: " << config->output.directory << "\n";
            return 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Clean error: " << e.what() << std::endl;
            return 1;
        }
    }

    // Handle --init command (create new .mtproj file)
    if (argc >= 4 && std::string(argv[1]) == "--init")
    {
        std::string projectName = argv[2];
        std::string includePath = argv[3];

        std::string filename = projectName + ".mtproj";

        if (std::filesystem::exists(filename))
        {
            std::cerr << "Error: " << filename << " already exists\n";
            return 1;
        }

        std::ofstream outFile(filename);
        if (!outFile)
        {
            std::cerr << "Error: Could not create " << filename << "\n";
            return 1;
        }

        outFile << "<Project Name=\"" << projectName << "\" Version=\"1.0.0\">\n";
        outFile << "  <Source>\n";
        outFile << "    <Include>" << includePath << "</Include>\n";
        outFile << "  </Source>\n";
        outFile << "  <Output Directory=\"build\" />\n";
        outFile << "  <Imports>\n";
        outFile << "  </Imports>\n";
        outFile << "</Project>\n";

        outFile.close();

        std::cout << "Created " << filename << "\n";
        return 0;
    }

    // Handle --add command (add include pattern to project)
    if (argc >= 3 && std::string(argv[1]) == "--add")
    {
        std::string pattern = argv[2];
        std::string mtprojPath;

        if (argc >= 4)
        {
            mtprojPath = argv[3];
        }
        else
        {
            project::ProjectConfigParser parser;
            auto found = parser.findProject(".");
            if (!found)
            {
                std::cerr << "Error: No .mtproj file found\n";
                return 1;
            }
            mtprojPath = *found;
        }

        try
        {
            std::ifstream inFile(mtprojPath);
            if (!inFile)
            {
                std::cerr << "Error: Could not open " << mtprojPath << "\n";
                return 1;
            }

            std::stringstream buffer;
            buffer << inFile.rdbuf();
            std::string content = buffer.str();
            inFile.close();

            // Find </Source> and insert before it
            std::string searchTag = "</Source>";
            size_t pos = content.find(searchTag);
            if (pos == std::string::npos)
            {
                std::cerr << "Error: Invalid .mtproj format (missing </Source>)\n";
                return 1;
            }

            std::string newInclude = "    <Include>" + pattern + "</Include>\n  ";
            content.insert(pos, newInclude);

            std::ofstream outFile(mtprojPath);
            outFile << content;
            outFile.close();

            std::cout << "Added include pattern: " << pattern << "\n";
            return 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

    // Handle --remove command (remove include pattern from project)
    if (argc >= 3 && std::string(argv[1]) == "--remove")
    {
        std::string pattern = argv[2];
        std::string mtprojPath;

        if (argc >= 4)
        {
            mtprojPath = argv[3];
        }
        else
        {
            project::ProjectConfigParser parser;
            auto found = parser.findProject(".");
            if (!found)
            {
                std::cerr << "Error: No .mtproj file found\n";
                return 1;
            }
            mtprojPath = *found;
        }

        try
        {
            std::ifstream inFile(mtprojPath);
            if (!inFile)
            {
                std::cerr << "Error: Could not open " << mtprojPath << "\n";
                return 1;
            }

            std::stringstream buffer;
            buffer << inFile.rdbuf();
            std::string content = buffer.str();
            inFile.close();

            // Find and remove the include line
            std::string searchPattern = "<Include>" + pattern + "</Include>";
            size_t pos = content.find(searchPattern);
            if (pos == std::string::npos)
            {
                std::cerr << "Error: Pattern not found in project: " << pattern << "\n";
                return 1;
            }

            // Find the start of the line (go back to newline or start)
            size_t lineStart = pos;
            while (lineStart > 0 && content[lineStart - 1] != '\n')
            {
                --lineStart;
            }

            // Find end of line
            size_t lineEnd = pos + searchPattern.length();
            while (lineEnd < content.length() && content[lineEnd] != '\n')
            {
                ++lineEnd;
            }
            if (lineEnd < content.length())
            {
                ++lineEnd; // Include the newline
            }

            content.erase(lineStart, lineEnd - lineStart);

            std::ofstream outFile(mtprojPath);
            outFile << content;
            outFile.close();

            std::cout << "Removed include pattern: " << pattern << "\n";
            return 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

    // Handle --compile command
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--compile")
        {
            std::string sourceFile;

            // Parse remaining arguments for filename
            for (int j = i + 1; j < argc; ++j)
            {
                std::string arg = argv[j];
                if (arg[0] != '-')
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
                std::cout << "Compiling " << sourceFile << " to " << outputFile << "...\n";

                ScriptInterpreter interpreter(constants::ExecutionMode::BYTECODE_VM);
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
            interpreter.getVM()->setJitEnabled(true);
            interpreter.runCompiledBytecode(cachedFile);
            return 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Execution error: " << e.what() << std::endl;
            return 1;
        }
    }

    // Handle --test-script-objects command
    if (argc >= 3 && std::string(argv[1]) == "--test-script-objects")
    {
        std::string scriptFile = argv[2];

        try
        {
            demonstrateScriptObjectUsage(scriptFile, constants::ExecutionMode::BYTECODE_VM);
            return 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

    // Handle --find-script-classes command
    if (argc >= 3 && std::string(argv[1]) == "--find-script-classes")
    {
        std::string scriptFile = argv[2];

        try
        {
            std::cout << "Analyzing script: " << scriptFile << "\n";
            std::cout << "(Parsing and registering classes for analysis)\n\n";

            ScriptInterpreter interpreter(constants::ExecutionMode::BYTECODE_VM);

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

    // Parse debug mode, gc-stats flag, jit flag, and filename
    std::string filename;
    bool debugMode = false;
    bool printGCStats = false;
    bool enableJit = true;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--debug")
        {
            debugMode = true;
        }
        else if (arg == "--gc-stats")
        {
            printGCStats = true;
        }
        else if (arg == "--no-jit")
        {
            enableJit = false;
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

    // Run in debug mode if --debug flag present
    if (debugMode)
    {
        runInDebugMode(filename, execMode);
        return 0;
    }

    try
    {
        ScriptInterpreter interpreter(execMode);

        if (enableJit)
        {
            interpreter.getVM()->setJitEnabled(true);
            std::cout << "Execution Mode: Bytecode VM + JIT\n\n";
        }
        else
        {
            std::cout << "Execution Mode: Bytecode VM\n\n";
        }

        interpreter.runScript(filename);

        // Force a GC collection at program end to detect any remaining cycles
        gc::GC::forceCollect();

        // Print GC statistics if requested
        if (printGCStats)
        {
            gc::GC::printStats();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        gc::GC::shutdown(); // Clean up GC before exit
        reflection::ReflectionNatives::cleanup();
        return 1;
    }

    // Clean up GC to avoid static destruction order issues
    gc::GC::shutdown();

    // Cleanup reflection static state to avoid static destruction order issues
    reflection::ReflectionNatives::cleanup();

    return 0;
}
