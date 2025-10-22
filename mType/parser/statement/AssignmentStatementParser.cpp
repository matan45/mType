#include "AssignmentStatementParser.hpp"
#include "../ExpressionParser.hpp"
#include "../TypeParser.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../../ast/nodes/statements/IndexAssignmentNode.hpp"
#include "../../ast/nodes/expressions/VariableNode.hpp"
#include "../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::statement
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::classes;
    using namespace token;
    using namespace errors;

    AssignmentStatementParser::AssignmentStatementParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx), expressionParser(nullptr)
    {
    }

    void AssignmentStatementParser::setExpressionParser(ExpressionParser& exprParser)
    {
        expressionParser = &exprParser;
    }

    std::unique_ptr<ASTNode> AssignmentStatementParser::parse()
    {
        throw ParseException("Invalid use of AssignmentStatementParser", tokenStream.current().location);
    }

    bool AssignmentStatementParser::canParse(const TokenStream& stream) const
    {
        return stream.check(TokenType::IDENTIFIER);
    }

    std::unique_ptr<ASTNode> AssignmentStatementParser::parseAssignment()
    {
        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException("Expected identifier in assignment", tokenStream.current().location);
        }

        std::string varName = tokenStream.current().stringValue.getString();
        SourceLocation varLocation = tokenStream.location();
        tokenStream.advance();

        // Check for compound assignment operators
        TokenType opType = tokenStream.current().type;
        if (isAssignmentOperator(opType))
        {
            tokenStream.advance();
            if (!expressionParser)
            {
                throw ParseException("ExpressionParser not initialized", tokenStream.current().location);
            }
            auto value = expressionParser->parseExpression();

            // For compound assignments, create a binary expression
            if (opType != TokenType::ASSIGN)
            {
                value = createCompoundAssignment(varName, varLocation, opType, std::move(value));
            }

            expectToken(TokenType::SEMICOLON);
            return std::make_unique<AssignmentNode>(varName, std::move(value), ValueType::VOID,
                                                    "", false, false, varLocation);
        }

        throw ParseException("Expected assignment operator", tokenStream.current().location);
    }

    std::unique_ptr<ASTNode> AssignmentStatementParser::parseExpressionStatement()
    {
        if (!expressionParser)
        {
            throw ParseException("ExpressionParser not initialized", tokenStream.current().location);
        }
        auto expr = expressionParser->parseExpression();

        // Check if this is a member assignment (obj.field = value)
        if (auto memberAccess = dynamic_cast<MemberAccessNode*>(expr.get()))
        {
            TokenType opType = tokenStream.current().type;
            if (isAssignmentOperator(opType))
            {
                errors::SourceLocation opLocation = tokenStream.current().location; // Capture location before advancing
                tokenStream.advance(); // consume assignment operator
                auto value = expressionParser->parseExpression();

                // For compound assignments, create a binary expression
                if (opType != TokenType::ASSIGN)
                {
                    TokenType binaryOp = getCorrespondingBinaryOperator(opType);
                    value = std::make_unique<BinaryExpNode>(std::move(expr), binaryOp, std::move(value), opLocation);

                    expectToken(TokenType::SEMICOLON);
                    return value;
                }

                expectToken(TokenType::SEMICOLON);

                // Extract the object and member name for the assignment
                auto object = memberAccess->getObjectShared();
                std::string memberName = memberAccess->getMemberName();

                return std::make_unique<MemberAssignmentNode>(object, memberName, std::move(value));
            }
        }

        // Check if this is an index assignment (array[index] = value)
        if (auto indexAccess = dynamic_cast<IndexAccessNode*>(expr.get()))
        {
            TokenType opType = tokenStream.current().type;
            if (isAssignmentOperator(opType))
            {
                errors::SourceLocation opLocation = tokenStream.current().location; // Capture location before advancing
                tokenStream.advance(); // consume assignment operator
                auto value = expressionParser->parseExpression();

                // For compound assignments, create a binary expression
                if (opType != TokenType::ASSIGN)
                {
                    TokenType binaryOp = getCorrespondingBinaryOperator(opType);
                    value = std::make_unique<BinaryExpNode>(std::move(expr), binaryOp, std::move(value), opLocation);

                    expectToken(TokenType::SEMICOLON);
                    return value;
                }

                expectToken(TokenType::SEMICOLON);

                // Extract the object and index for the assignment
                auto object = indexAccess->transferCollectionOwnership();
                auto index = indexAccess->transferIndexOwnership();

                return std::make_unique<IndexAssignmentNode>(std::move(object), std::move(index), std::move(value));
            }
        }

        expectToken(TokenType::SEMICOLON);
        return expr;
    }

    bool AssignmentStatementParser::isAssignmentOperator(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::ASSIGN:
        case TokenType::PLUS_ASSIGN:
        case TokenType::MINUS_ASSIGN:
        case TokenType::MULTIPLY_ASSIGN:
        case TokenType::DIVIDE_ASSIGN:
        case TokenType::MODULO_ASSIGN:
            return true;
        default:
            return false;
        }
    }

    TokenType AssignmentStatementParser::getCorrespondingBinaryOperator(TokenType assignmentOp) const
    {
        switch (assignmentOp)
        {
        case TokenType::PLUS_ASSIGN:
            return TokenType::PLUS;
        case TokenType::MINUS_ASSIGN:
            return TokenType::MINUS;
        case TokenType::MULTIPLY_ASSIGN:
            return TokenType::MULTIPLY;
        case TokenType::DIVIDE_ASSIGN:
            return TokenType::DIVIDE;
        case TokenType::MODULO_ASSIGN:
            return TokenType::MODULO;
        default:
            return TokenType::PLUS; // fallback
        }
    }

    std::unique_ptr<ASTNode> AssignmentStatementParser::createCompoundAssignment(
        const std::string& varName,
        const SourceLocation& location,
        TokenType opType,
        std::unique_ptr<ASTNode> value)
    {
        auto varNode = std::make_unique<VariableNode>(varName, location);
        TokenType binaryOp = getCorrespondingBinaryOperator(opType);

        return std::make_unique<BinaryExpNode>(std::move(varNode), binaryOp, std::move(value), location);
    }
}
