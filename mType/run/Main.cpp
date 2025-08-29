#include "../tests/testFramework/TestSuite.hpp"
#include "../tests/suites/ControlFlowTestSuite.hpp"
#include "../tests/suites/ClassTestSuite.hpp"
#include "../tests/suites/ImportTestSuite.hpp"
#include "../tests/suites/IntegrationTestSuite.hpp"
#include "../tests/suites/NameSpaceTestSuite.hpp"
#include "../tests/suites/TypeCheckingTestSuite.hpp"
#include "../tests/suites/ErrorTestSuite.hpp"

#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../evaluator/Evaluator.hpp"
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


void runTests()
{
    std::cout << "Running all test suites...\n\n";

    std::vector<std::unique_ptr<TestSuite>> suites;
    suites.push_back(std::make_unique<ControlFlowTestSuite>());
    suites.push_back(std::make_unique<ImportTestSuite>());
    /*suites.push_back(std::make_unique<ClassTestSuite>());
    suites.push_back(std::make_unique<IntegrationTestSuite>());
    suites.push_back(std::make_unique<NameSpaceTestSuite>());
    suites.push_back(std::make_unique<TypeCheckingTestSuite>());
    suites.push_back(std::make_unique<ErrorTestSuite>());*/

    for (auto& suite : suites)
    {
        suite->setupTests();  // Initialize test cases
        suite->run();         // Run tests and generate reports
        
    }
    
    // Print final summary
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "ALL TEST SUITES COMPLETED" << std::endl;
    std::cout << "Reports generated in test_reports/ directory" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
}


int main(int argc, char* argv[])
{
    runTests();
    if (argc == 2 && std::string(argv[1]) == "--tests") {
        runTests();
        return 0;
    }
    
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <script_file.mt> or --tests" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    
    try {
        ScriptInterpreter interpreter;
        interpreter.runScript(filename);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
