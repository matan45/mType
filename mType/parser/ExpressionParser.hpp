#pragma once
#include <memory>
#include <vector>
#include "../ast/ASTNode.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"

namespace parser
{
    class ParseContext;
    using namespace ast;
    
    class ExpressionParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;
        
    public:
        explicit ExpressionParser(TokenStream& stream, ParseContext& ctx) : tokenStream(stream), context(ctx) {}
        
        // Expression parsing methods (precedence climbing)
        std::unique_ptr<ASTNode> parseExpression();
        std::unique_ptr<ASTNode> parseAssignment();
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

        // Argument parsing (used by other parsers)
        std::vector<std::unique_ptr<ASTNode>> parseArguments();

        // Generic type argument parsing for static method calls
        std::vector<std::string> parseGenericTypeArguments();
        
    private:
        // Helper methods
        std::unique_ptr<ASTNode> parseMemberAccess(std::unique_ptr<ASTNode> object);
        std::unique_ptr<ASTNode> parseIndexAccess(std::unique_ptr<ASTNode> collection);
        std::unique_ptr<ASTNode> parseArrayLiteral();
        std::string parseGenericTypeArgument();
    
    };
}

