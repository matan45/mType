#include "ScriptInterpreter.hpp"
#include "../parser/Parser.hpp"
#include "../lexer/Lexer.hpp"
#include "../evaluator/Evaluator.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>

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
        
        parser::Parser parser(lexer);
        auto ast = parser.parseProgram();
        
        environment::EnvironmentBuilder envBuilder;
        auto environment = envBuilder.build();
        
        evaluator::Evaluator evaluator(environment);
        evaluator.evaluate(ast.get());
    }
}
