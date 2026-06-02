#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "../ast/ASTNode.hpp"
#include "../errors/SourceLocation.hpp"
#include "../token/Token.hpp"
#include "../token/TokenType.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"

namespace parser
{
    using namespace ast;

    class ExpressionParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;

        void expectToken(token::TokenType type);
        bool tryConsumeToken(token::TokenType type);
        const token::Token& currentToken() const;
        bool isAtEnd() const;

    public:
        explicit ExpressionParser(TokenStream& stream, ParseContext& ctx);

        // Public precedence-climbing entry points (called from other parsers)
        std::unique_ptr<ASTNode> parseExpression();
        std::unique_ptr<ASTNode> parseTernary();  // LambdaParser uses this to skip the assignment level
        std::unique_ptr<ASTNode> parseUnary();
        std::unique_ptr<ASTNode> parsePostfix();
        std::unique_ptr<ASTNode> parsePrimary();

        // Used by ClassParser/StatementParser via context (parseNewExpression path)
        std::vector<std::unique_ptr<ASTNode>> parseArguments();
        std::vector<std::string> parseGenericTypeArguments();

    private:
        // Assignment + precedence ladder
        std::unique_ptr<ASTNode> parseAssignment();
        std::unique_ptr<ASTNode> parseLambda();
        std::unique_ptr<ASTNode> parseLogicalOr();
        std::unique_ptr<ASTNode> parseLogicalAnd();
        std::unique_ptr<ASTNode> parseBitwiseOr();
        std::unique_ptr<ASTNode> parseBitwiseXor();
        std::unique_ptr<ASTNode> parseBitwiseAnd();
        std::unique_ptr<ASTNode> parseEquality();
        std::unique_ptr<ASTNode> parseComparison();
        std::unique_ptr<ASTNode> parseIsClassOf();
        std::unique_ptr<ASTNode> parseShift();
        std::unique_ptr<ASTNode> parseAdditive();
        std::unique_ptr<ASTNode> parseMultiplicative();
        std::unique_ptr<ASTNode> parseBinaryLevel(
            std::function<std::unique_ptr<ASTNode>()> parseNext,
            const std::vector<token::TokenType>& operators);

        // Cast / instanceof (absorbed from CastParser)
        bool isCastExpression() const;
        bool canParseCast() const;
        std::unique_ptr<ASTNode> parseCastExpression();
        std::unique_ptr<ASTNode> parseInstanceOfExpression(std::unique_ptr<ASTNode> left);

        // Postfix (absorbed from PostfixOperatorParser)
        std::unique_ptr<ASTNode> parsePostfixOperations(std::unique_ptr<ASTNode> expr);
        std::unique_ptr<ASTNode> parseFunctionCall(std::unique_ptr<ASTNode> expr);
        std::unique_ptr<ASTNode> parseMemberAccess(std::unique_ptr<ASTNode> object, bool isSafe = false);
        std::unique_ptr<ASTNode> parseIndexAccess(std::unique_ptr<ASTNode> collection);
        std::unique_ptr<ASTNode> parseScopeResolution(std::unique_ptr<ASTNode> expr);
        std::unique_ptr<ASTNode> parseGenericMethodCall(const std::string& className,
                                                        const std::string& methodName,
                                                        const std::vector<std::string>& typeArgs);
        bool isPostfixOperator(token::TokenType type) const noexcept;
        bool isGenericFunctionCall();

        // Literal / primary (absorbed from LiteralParser)
        std::unique_ptr<ASTNode> parseIntegerLiteral();
        std::unique_ptr<ASTNode> parseFloatLiteral();
        std::unique_ptr<ASTNode> parseStringLiteral();
        std::unique_ptr<ASTNode> parseBooleanLiteral();
        std::unique_ptr<ASTNode> parseNullLiteral();
        std::unique_ptr<ASTNode> parseIdentifier();
        std::unique_ptr<ASTNode> parseParenthesizedExpression();
        std::unique_ptr<ASTNode> parseSuperExpression();
        std::unique_ptr<ASTNode> parseInterpolatedStringExpression();
        std::unique_ptr<ASTNode> parseArrayLiteral();

        // Argument lists (absorbed from ArgumentParser)
        std::vector<std::unique_ptr<ASTNode>> parseArgumentsWithParentheses();
        std::string parseGenericTypeArgument();

        // Lambda detection
        bool isLambdaStart() const;
        bool isLikelyLambdaParameterList() const;

        // Assignment handling helpers
        std::unique_ptr<ASTNode> handleMemberAssignment(
            ast::nodes::classes::MemberAccessNode* memberAccessNode,
            token::TokenType opType,
            std::unique_ptr<ASTNode> rightExpr,
            const errors::SourceLocation& location);
        std::unique_ptr<ASTNode> handleSuperMemberAssignment(
            ast::nodes::classes::SuperMemberAccessNode* superMemberAccessNode,
            token::TokenType opType,
            std::unique_ptr<ASTNode> rightExpr,
            const errors::SourceLocation& location);
        std::unique_ptr<ASTNode> handleIndexAssignment(
            ast::nodes::expressions::IndexAccessNode* indexAccessNode,
            token::TokenType opType,
            std::unique_ptr<ASTNode> rightExpr,
            const errors::SourceLocation& location);
        std::unique_ptr<ASTNode> handleVariableAssignment(
            ast::nodes::expressions::VariableNode* variableNode,
            token::TokenType opType,
            std::unique_ptr<ASTNode> rightExpr,
            const errors::SourceLocation& location);
    };
}
