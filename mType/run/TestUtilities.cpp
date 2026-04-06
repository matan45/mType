#include "TestUtilities.hpp"

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
#include "../tests/suites/NullSafetyTestSuite.hpp"
#include "../tests/suites/JsonTestSuite.hpp"
#include "../tests/suites/StringInterpolationTestSuite.hpp"

#include "../gc/GC.hpp"
#include "../services/ScriptInterpreter.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../reflection/ReflectionNatives.hpp"
#include "../json/JsonNatives.hpp"

#include <vector>
#include <memory>
#include <iostream>
#include <string>

using namespace tests::testSuite;
using namespace tests::testFramework;
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
    else if (suiteName == "nullsafety" || suiteName == "null-safety" || suiteName == "null")
    {
        return std::make_unique<NullSafetyTestSuite>();
    }
    else if (suiteName == "json")
    {
        return std::make_unique<JsonTestSuite>();
    }
    else if (suiteName == "interpolation" || suiteName == "string-interpolation")
    {
        return std::make_unique<StringInterpolationTestSuite>();
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
    std::cout << "  null-safety  - Null Safety Test Suite\n";
    std::cout << "  json         - JSON Serialization/Deserialization Test Suite\n";
    std::cout << "  interpolation - String Interpolation Test Suite\n";
    std::cout << "  native       - Native C++ Integration Test Suite\n";
}

void runSpecificTestSuite(const std::string& suiteName,
                          constants::ExecutionMode execMode)
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
    json::JsonNatives::cleanup();
}

void runAllTests(constants::ExecutionMode execMode)
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
    suites.push_back(std::make_unique<CollectionsTestSuite>());
    suites.push_back(std::make_unique<GCTestSuite>());
    suites.push_back(std::make_unique<NullSafetyTestSuite>());
    suites.push_back(std::make_unique<JsonTestSuite>());
    suites.push_back(std::make_unique<StringInterpolationTestSuite>());

    for (auto& suite : suites)
    {
        suite->setupTests(); // Initialize test cases
        suite->setExecutionModeForAll(execMode); // Set execution mode
        suite->run(); // Run tests and generate reports
    }

    // Cleanup reflection static state to avoid static destruction order issues
    reflection::ReflectionNatives::cleanup();
    json::JsonNatives::cleanup();

    // Print final summary
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "ALL TEST SUITES COMPLETED" << std::endl;
    std::cout << "Reports generated in test_reports/ directory" << std::endl;

    std::cout << std::string(80, '=') << std::endl;
}
