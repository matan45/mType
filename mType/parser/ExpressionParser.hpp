#pragma once
#include <memory>
#include <vector>
#include "../ast/ASTNode.hpp"

namespace parser
{
    class Parser;
    using namespace ast;
    
    class ExpressionParser
    {
    private:
        Parser& parser;
        
    public:
        explicit ExpressionParser(Parser& p) : parser(p) {}
        
        // Expression parsing methods (precedence climbing)
        std::unique_ptr<ASTNode> parseExpression();
        std::unique_ptr<ASTNode> parseTernary();
        std::unique_ptr<ASTNode> parseLogicalOr();
        std::unique_ptr<ASTNode> parseLogicalAnd();
        std::unique_ptr<ASTNode> parseEquality();
        std::unique_ptr<ASTNode> parseComparison();
        std::unique_ptr<ASTNode> parseAdditive();
        std::unique_ptr<ASTNode> parseMultiplicative();
        std::unique_ptr<ASTNode> parseUnary();
        std::unique_ptr<ASTNode> parsePostfix();
        std::unique_ptr<ASTNode> parsePrimary();

    private:
        // Helper methods
        std::vector<std::unique_ptr<ASTNode>> parseArguments();
        std::unique_ptr<ASTNode> parseMemberAccess(std::unique_ptr<ASTNode> object);
    
    };
}

