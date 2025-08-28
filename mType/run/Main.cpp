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

void printUsage(const std::string& programName) {
    std::cout << "Usage:\n";
    std::cout << "  " << programName << " [file.mt]  - Parse and test a specific .mt file\n";
    std::cout << "  " << programName << " --tests    - Run all test suites\n";
    std::cout << "  " << programName << "            - Run all test suites (default)\n";
}

void printASTNode(ast::ASTNode* node, int depth = 0) {
    if (!node) return;
    
    std::string indent(depth * 2, ' ');
    std::cout << indent << "- " << typeid(*node).name() << std::endl;
    
    // For program nodes, print their statements
    if (auto program = dynamic_cast<ast::nodes::statements::ProgramNode*>(node)) {
        for (const auto& stmt : program->getStatements()) {
            printASTNode(stmt.get(), depth + 1);
        }
    }
}

bool parseFile(const std::string& filePath) {
    try {
        // Check if file exists
        if (!std::filesystem::exists(filePath)) {
            std::cerr << "Error: File '" << filePath << "' does not exist.\n";
            return false;
        }
        
        // Read file content
        std::ifstream file(filePath);
        if (!file.is_open()) {
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
        
        if (ast) {
            std::cout << "Parsing successful!\n";
            std::cout << "AST Structure:\n";
            printASTNode(ast.get());
            std::cout << "\nParsing completed successfully.\n";
            
            // Now evaluate the AST
            std::cout << "\n=== EVALUATION OUTPUT ===\n";
            try {
                auto env = EnvironmentBuilder::createDefault();
                Evaluator evaluator(env);
                evaluator.evaluate(ast.get());
                std::cout << "=== EVALUATION COMPLETED ===\n";
            } catch (const std::exception& e) {
                std::cerr << "Evaluation error: " << e.what() << std::endl;
                return false;
            }
            
            return true;
        } else {
            std::cout << "Parsing failed: No AST generated.\n";
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing file: " << e.what() << std::endl;
        return false;
    }
}

void runTests() {
    std::cout << "Running all test suites...\n\n";
    
    std::vector<std::unique_ptr<TestSuite>> suites;
    suites.push_back(std::make_unique<ControlFlowTestSuite>());
    suites.push_back(std::make_unique<ImportTestSuite>());
    suites.push_back(std::make_unique<ClassTestSuite>());
    suites.push_back(std::make_unique<IntegrationTestSuite>());
    suites.push_back(std::make_unique<NameSpaceTestSuite>());
    suites.push_back(std::make_unique<TypeCheckingTestSuite>());
    suites.push_back(std::make_unique<ErrorTestSuite>());

    for (auto& suite : suites)
    {
        suite->run();
    }
}

int main(int argc, char* argv[])
{
    if (argc == 1) {
        // No arguments - run tests
        runTests();
    } else if (argc == 2) {
        std::string arg = argv[1];
        if (arg == "--tests") {
            runTests();
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
        } else {
            // Assume it's a file path
            if (!parseFile(arg)) {
                return 1;
            }
        }
    } else {
        std::cerr << "Error: Invalid number of arguments.\n";
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}
