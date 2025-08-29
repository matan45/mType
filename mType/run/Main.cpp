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
#include "../services/ImportManager.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../evaluator/Evaluator.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../errors/UndefinedException.hpp"
#include "../errors/TypeException.hpp"
#include "../errors/ParseException.hpp"

#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>


using namespace tests::testSuite;
using namespace tests::testFramework;
using namespace parser;
using namespace lexer;
using namespace services;
using namespace evaluator;
using namespace environment;

void printUsage(const std::string& programName)
{
    std::cout << "Usage:\n";
    std::cout << "  " << programName << " [file.mt]  - Parse and test a specific .mt file\n";
    std::cout << "  " << programName << " --tests    - Run all test suites\n";
    std::cout << "  " << programName << "            - Run all test suites (default)\n";
}

void printASTNode(ast::ASTNode* node, int depth = 0)
{
    if (!node) return;

    std::string indent(depth * 2, ' ');
    std::cout << indent << "- " << typeid(*node).name() << std::endl;

    // For program nodes, print their statements
    if (auto program = dynamic_cast<ast::nodes::statements::ProgramNode*>(node))
    {
        for (const auto& stmt : program->getStatements())
        {
            printASTNode(stmt.get(), depth + 1);
        }
    }
}

bool parseFile(const std::string& filePath)
{
    try
    {
        // Check if file exists
        if (!std::filesystem::exists(filePath))
        {
            std::cerr << "Error: File '" << filePath << "' does not exist.\n";
            return false;
        }

        // Read file content
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            std::cerr << "Error: Could not open file '" << filePath << "'.\n";
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();

        std::cout << "Parsing file: " << filePath << std::endl;
        std::cout << "File content:\n" << content << std::endl;
        std::cout << "================================\n\n";

        // Initialize lexer and parser
        Lexer lexer(content);
        auto importManager = std::make_shared<ImportManager>();
        Parser parser(lexer);
        parser.setImportManager(importManager.get());

        // Parse the file
        auto ast = parser.parseProgram();

        if (ast)
        {
            std::cout << "Parsing successful!\n";
            std::cout << "AST Structure:\n";
            printASTNode(ast.get());
            std::cout << "\nParsing completed successfully.\n";

            // Now evaluate the AST
            std::cout << "\n=== EVALUATION OUTPUT ===\n";
            try
            {
                auto env = EnvironmentBuilder::createDefault();
                Evaluator evaluator(env);
                evaluator.evaluate(ast.get());
                std::cout << "=== EVALUATION COMPLETED ===\n";
            }
            catch (const std::exception& e)
            {
                std::cerr << "Evaluation error: " << e.what() << std::endl;
                return false;
            }

            return true;
        }
        else
        {
            std::cout << "Parsing failed: No AST generated.\n";
            return false;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing file: " << e.what() << std::endl;
        return false;
    }
}

void runTests()
{
    std::cout << "Running all test suites...\n\n";

    std::vector<std::unique_ptr<TestSuite>> suites;
    suites.push_back(std::make_unique<ControlFlowTestSuite>());
    suites.push_back(std::make_unique<ImportTestSuite>());
    suites.push_back(std::make_unique<ClassTestSuite>());
    suites.push_back(std::make_unique<IntegrationTestSuite>());
    suites.push_back(std::make_unique<NameSpaceTestSuite>());
    suites.push_back(std::make_unique<TypeCheckingTestSuite>());
    suites.push_back(std::make_unique<ErrorTestSuite>());

    // Track overall statistics
    int totalSuitesPassed = 0;
    int totalSuitesFailed = 0;

    for (auto& suite : suites)
    {
        suite->setupTests();  // Initialize test cases
        suite->run();         // Run tests and generate reports
        
        // You could add logic here to track suite-level pass/fail
        // based on the runner's results
    }
    
    // Print final summary
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "ALL TEST SUITES COMPLETED" << std::endl;
    std::cout << "Reports generated in test_reports/ directory" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
}

void safeEvaluation(const std::string& code)
{
    try
    {
        auto env = EnvironmentBuilder::createDefault();
        Evaluator evaluator(env);

        Lexer lexer(code);
        Parser parser(lexer);
        auto ast = parser.parseProgram();

        Value result = evaluator.evaluate(ast.get());

        // Handle different Value types using std::visit
        std::visit([](const auto& value)
        {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, int>)
            {
                std::cout << "Result (int): " << value << std::endl;
            }
            else if constexpr (std::is_same_v<T, float>)
            {
                std::cout << "Result (float): " << value << std::endl;
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                std::cout << "Result (bool): " << (value ? "true" : "false") << std::endl;
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                std::cout << "Result (string): " << value << std::endl;
            }
            else if constexpr (std::is_same_v<T, std::shared_ptr<runtimeTypes::klass::ObjectInstance>>)
            {
                std::cout << "Result (object): " << value->getTypeName() << std::endl;
            }
            else if constexpr (std::is_same_v<T, std::monostate>)
            {
                std::cout << "Result: void" << std::endl;
            }
            else if constexpr (std::is_same_v<T, nullptr_t>)
            {
                std::cout << "Result: null" << std::endl;
            }
        }, result);
    }
    catch (const errors::ParseException& e)
    {
        std::cerr << "Parse error: " << e.what() << std::endl;
    }
    catch (const errors::TypeException& e)
    {
        std::cerr << "Type error: " << e.what() << std::endl;
    }
    catch (const errors::UndefinedException& e)
    {
        std::cerr << "Undefined error: " << e.what() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Runtime error: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        // No arguments - run tests
        runTests();
    }
    else if (argc == 2)
    {
        std::string arg = argv[1];
        if (arg == "--tests")
        {
            runTests();
        }
        else if (arg == "--help" || arg == "-h")
        {
            printUsage(argv[0]);
        }
        else
        {
            // Assume it's a file path
            if (!parseFile(arg))
            {
                return 1;
            }
        }
    }
    else
    {
        std::cerr << "Error: Invalid number of arguments.\n";
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
