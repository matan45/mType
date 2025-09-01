#include "Parser.hpp"
#include "../services/ImportManager.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/expressions/StringNode.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../ast/nodes/statements/ImportedDeclarationNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace token;
    using namespace errors;

    Parser::Parser(Lexer& lex, std::unique_ptr<services::ImportManager> manager)
        : lexer(lex), currentToken(lexer.getNextToken()), importManager(std::move(manager))
    {
        // Initialize subparsers with reference to main parser
        statementParser = std::make_unique<StatementParser>(*this);
        expressionParser = std::make_unique<ExpressionParser>(*this);
        classParser = std::make_unique<ClassParser>(*this);
    }

    std::unique_ptr<ASTNode> Parser::parseProgram()
    {
        auto program = std::make_unique<ProgramNode>(currentToken.location);

        while (currentToken.type != TokenType::END)
        {
            auto statement = parseStatement();
            if (statement)
            {
                // All statements, including import nodes, are added to the program
                // Import processing is completely deferred to evaluation phase
                program->addStatement(std::move(statement));
            }
        }

        return std::move(program);
    }

    void Parser::advance()
    {
        currentToken = lexer.getNextToken();
    }

    bool Parser::match(TokenType type)
    {
        if (currentToken.type == type)
        {
            advance();
            return true;
        }
        return false;
    }

    void Parser::expect(TokenType type)
    {
        if (!match(type))
        {
            throw ParseException("Expected token type " + std::to_string(static_cast<int>(type)),
                                 currentToken.location);
        }
    }

    std::unique_ptr<ASTNode> Parser::parseStatement()
    {
        // Delegate to StatementParser
        return statementParser->parseStatement();
    }

    std::unique_ptr<ASTNode> Parser::parseExpression()
    {
        // Delegate to ExpressionParser
        return expressionParser->parseExpression();
    }

    void Parser::handleImportStatement(nodes::statements::ImportNode* importNode,
                                       nodes::statements::ProgramNode* program)
    {
        if (!importManager)
        {
            throw ParseException("ImportManager not set - cannot process import statement", importNode->getLocation());
        }

        // Get the imported AST (may be null if import resolution is deferred to evaluation time)
        ASTNode* importedAST = importNode->getImportedAST();
        if (!importedAST)
        {
            return; // Nothing to import
        }

        // INLINE APPROACH: Extract declarations from imported AST and add them directly
        // Cast to ProgramNode (imported files are parsed as programs)
        if (auto importedProgram = dynamic_cast<ProgramNode*>(importedAST))
        {
            // Get all statements from the imported program
            const auto& importedStatements = importedProgram->getStatements();

            // Extract and inline importable declarations
            for (const auto& stmt : importedStatements)
            {
                // TODO: Import functionality temporarily disabled during namespace removal
                // Will re-implement import processing in evaluation phase
                /*
                if (isImportableDeclaration(stmt.get()))
                {
                    auto importedDecl = std::make_unique<ImportedDeclarationNode>(
                        stmt.get(), 
                        importNode->getFilePath(),
                        stmt->getLocation() 
                    );
                    program->addStatement(std::move(importedDecl));
                }
                */
            }
        }
    }

    bool Parser::isImportableDeclaration(ASTNode* node)
    {
        // Import these types of declarations:
        // - Function definitions (including native functions)
        // - Global variable declarations (assignments at top level)
        // - Class definitions
        // - Namespace definitions
        // 
        // Don't import:
        // - Import statements (to avoid recursive imports)
        // - Other statement types that aren't declarations

        // Skip import statements to avoid recursion
        if (dynamic_cast<ImportNode*>(node) != nullptr)
        {
            return false;
        }

        return dynamic_cast<ast::nodes::functions::FunctionNode*>(node) != nullptr ||
            dynamic_cast<ast::nodes::classes::ClassNode*>(node) != nullptr ||
            dynamic_cast<AssignmentNode*>(node) != nullptr; // Global variables
    }

    bool Parser::isAssignmentOperator(TokenType tokenType)
    {
        return tokenType == TokenType::ASSIGN ||
            tokenType == TokenType::PLUS_ASSIGN ||
            tokenType == TokenType::MINUS_ASSIGN ||
            tokenType == TokenType::MULTIPLY_ASSIGN ||
            tokenType == TokenType::DIVIDE_ASSIGN ||
            tokenType == TokenType::MODULO_ASSIGN;
    }

    ValueType Parser::parseType()
    {
        TokenType currentType = getCurrentToken().type;

        // Handle dedicated type tokens
        if (currentType == TokenType::INT)
        {
            advanceToken();
            return ValueType::INT;
        }
        else if (currentType == TokenType::FLOAT)
        {
            advanceToken();
            return ValueType::FLOAT;
        }
        else if (currentType == TokenType::BOOL)
        {
            advanceToken();
            return ValueType::BOOL;
        }
        else if (currentType == TokenType::STRING_TYPE)
        {
            advanceToken();
            return ValueType::STRING;
        }
        else if (currentType == TokenType::VOID)
        {
            advanceToken();
            return ValueType::VOID;
        }
        else if (currentType == TokenType::IDENTIFIER)
        {
            std::string typeName = getCurrentToken().stringValue;
            advanceToken();

            // Handle qualified names like geometry::Point
            while (getCurrentToken().type == TokenType::SCOPE)
            {
                advanceToken();
                if (getCurrentToken().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected identifier after '::'", getCurrentToken().location);
                }
                typeName += "::" + getCurrentToken().stringValue;
                advanceToken();
            }

            // Check if it's a string-based primitive type
            if (typeName == "int") return ValueType::INT;
            else if (typeName == "float") return ValueType::FLOAT;
            else if (typeName == "string") return ValueType::STRING;
            else if (typeName == "bool") return ValueType::BOOL;
            else if (typeName == "void") return ValueType::VOID;
            else
            {
                // Treat unknown identifier types as custom class types (OBJECT)
                return ValueType::OBJECT;
            }
        }
        else
        {
            throw ParseException("Expected type name", getCurrentToken().location);
        }
    }

    std::pair<ValueType, std::string> Parser::parseTypeWithClassName()
    {
        TokenType currentType = getCurrentToken().type;

        // Handle dedicated type tokens
        if (currentType == TokenType::INT)
        {
            advanceToken();
            return {ValueType::INT, ""};
        }
        else if (currentType == TokenType::FLOAT)
        {
            advanceToken();
            return {ValueType::FLOAT, ""};
        }
        else if (currentType == TokenType::BOOL)
        {
            advanceToken();
            return {ValueType::BOOL, ""};
        }
        else if (currentType == TokenType::STRING_TYPE)
        {
            advanceToken();
            return {ValueType::STRING, ""};
        }
        else if (currentType == TokenType::VOID)
        {
            advanceToken();
            return {ValueType::VOID, ""};
        }
        else if (currentType == TokenType::IDENTIFIER)
        {
            std::string typeName = getCurrentToken().stringValue;
            advanceToken();

            // Handle qualified names like geometry::Point
            while (getCurrentToken().type == TokenType::SCOPE)
            {
                advanceToken();
                if (getCurrentToken().type != TokenType::IDENTIFIER)
                {
                    throw ParseException("Expected identifier after '::'", getCurrentToken().location);
                }
                typeName += "::" + getCurrentToken().stringValue;
                advanceToken();
            }

            // Check if it's a string-based primitive type
            if (typeName == "int") return {ValueType::INT, ""};
            else if (typeName == "float") return {ValueType::FLOAT, ""};
            else if (typeName == "string") return {ValueType::STRING, ""};
            else if (typeName == "bool") return {ValueType::BOOL, ""};
            else if (typeName == "void") return {ValueType::VOID, ""};
            else
            {
                // Return OBJECT type with the actual class name
                return {ValueType::OBJECT, typeName};
            }
        }
        else
        {
            throw ParseException("Expected type name", getCurrentToken().location);
        }
    }

}
