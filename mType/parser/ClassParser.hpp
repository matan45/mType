#pragma once
#include <memory>
#include "../ast/ASTNode.hpp"

namespace parser
{
    class Parser;
    using namespace ast;
    class ClassParser
    {
    private:
        Parser& parser;
        
    public:
        explicit ClassParser(Parser& p) : parser(p) {}
        
        // Class parsing methods
        std::unique_ptr<ASTNode> parseClass();
        std::unique_ptr<ASTNode> parseConstructor();
        std::unique_ptr<ASTNode> parseMethod();
        std::unique_ptr<ASTNode> parseField();
        std::unique_ptr<ASTNode> parseNewExpression();
        
    private:
        bool isMethodOrConstructor();
    
    };
}

