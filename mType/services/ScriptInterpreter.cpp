#include "ScriptInterpreter.hpp"
#include "ImportManager.hpp"
#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../evaluator/Evaluator.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <memory>
#include <filesystem>

namespace services
{
    void ScriptInterpreter::runScript(const std::string& filename)
    {
        // Read the script file
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // Parse and execute
        lexer::Lexer lexer(content, filename);
        
        // Create and configure ImportManager
        auto importManager = std::make_shared<ImportManager>();
        
        // Set base directory to the directory of the script file
        std::filesystem::path scriptPath(filename);
        importManager->setBaseDirectory(scriptPath.parent_path().string());
        
        parser::Parser parser(lexer);
        auto ast = parser.parseProgram();
        
        environment::EnvironmentBuilder envBuilder;
        auto environment = envBuilder.build();
        
        // Set ImportManager on environment for clean architecture
        environment->setImportManager(importManager.get());
        
        evaluator::Evaluator evaluator(environment);
        evaluator.evaluate(ast.get());
    }
}
