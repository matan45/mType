#include "StatementParser.hpp"
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include "ExpressionParser.hpp"
#include "TypeParser.hpp"
#include "class/GenericParameterParser.hpp"
#include "utilities/AnnotationParser.hpp"
#include "utilities/AsyncValidator.hpp"
#include "utilities/ParameterParser.hpp"
#include "utilities/NameValidator.hpp"
#include "utilities/ParserUtils.hpp"
#include "utilities/VisibilityParser.hpp"
#include "../ast/AccessModifier.hpp"
#include "../ast/GenericType.hpp"
#include "../ast/GenericTypeParameter.hpp"
#include "../ast/nodes/statements/AssignmentNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/statements/BreakNode.hpp"
#include "../ast/nodes/statements/CaseNode.hpp"
#include "../ast/nodes/statements/ContinueNode.hpp"
#include "../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../ast/nodes/statements/DoWhileNode.hpp"
#include "../ast/nodes/statements/ForEachNode.hpp"
#include "../ast/nodes/statements/ForNode.hpp"
#include "../ast/nodes/statements/IfNode.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../ast/nodes/statements/IndexAssignmentNode.hpp"
#include "../ast/nodes/statements/MatchCaseNode.hpp"
#include "../ast/nodes/statements/MatchDefaultNode.hpp"
#include "../ast/nodes/statements/MatchNode.hpp"
#include "../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../ast/nodes/statements/SwitchNode.hpp"
#include "../ast/nodes/statements/WhileNode.hpp"
#include "../ast/nodes/functions/FunctionCallNode.hpp"
#include "../ast/nodes/functions/FunctionNode.hpp"
#include "../ast/nodes/functions/ReturnNode.hpp"
#include "../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../ast/nodes/expressions/BoolNode.hpp"
#include "../ast/nodes/expressions/FloatNode.hpp"
#include "../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../ast/nodes/expressions/IntegerNode.hpp"
#include "../ast/nodes/expressions/StringNode.hpp"
#include "../ast/nodes/expressions/VariableNode.hpp"
#include "../ast/nodes/classes/MemberAccessNode.hpp"
#include "../ast/nodes/classes/NewNode.hpp"
#include "../ast/nodes/classes/SuperMemberAccessNode.hpp"
#include "../ast/nodes/classes/SuperMemberAssignmentNode.hpp"
#include "../errors/DuplicateDeclarationException.hpp"
#include "../errors/MissingSemicolonException.hpp"
#include "../errors/ParseException.hpp"

namespace parser
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::classes;
    using namespace token;
    using namespace value;
    using namespace errors;
    using namespace parser::utilities;

    // ============================================================
    // Construction and BaseParser-equivalent helpers
    // ============================================================

    StatementParser::StatementParser(TokenStream& stream, ParseContext& ctx)
        : tokenStream(stream), context(ctx), expressionParser(nullptr)
    {
    }

    void StatementParser::expectToken(TokenType type)
    {
        if (!tokenStream.check(type))
        {
            if (type == TokenType::SEMICOLON)
            {
                throw MissingSemicolonException(tokenStream.current().location);
            }
            throw ParseException("Expected token but found different token", tokenStream.current().location);
        }
        tokenStream.advance();
    }

    bool StatementParser::tryConsumeToken(TokenType type)
    {
        if (tokenStream.check(type))
        {
            tokenStream.advance();
            return true;
        }
        return false;
    }

    const Token& StatementParser::currentToken() const
    {
        return tokenStream.current();
    }

    bool StatementParser::isAtEnd() const
    {
        return tokenStream.isAtEnd();
    }

    // ============================================================
    // Public dispatch
    // ============================================================

    std::unique_ptr<ASTNode> StatementParser::parseStatement()
    {
        StatementType type = StatementTypeDetector::detectStatementType(tokenStream);

        switch (type)
        {
        case StatementType::CLASS:
            return context.parseClass();
        case StatementType::INTERFACE:
            return context.parseInterface();
        case StatementType::ANNOTATION_DECLARATION:
            return context.parseAnnotationDeclaration();
        case StatementType::BLOCK:
            return parseBlock();
        case StatementType::EMPTY:
            tokenStream.advance();
            return nullptr;
        default:
            return delegateToSpecializedParser(type);
        }
    }

    std::unique_ptr<ASTNode> StatementParser::parseBlock()
    {
        auto blockLocation = tokenStream.current().location;
        tokenStream.expect(TokenType::LBRACE);
        auto block = std::make_unique<BlockNode>(blockLocation);

        while (!tokenStream.check(TokenType::RBRACE) && !tokenStream.isAtEnd())
        {
            auto statement = parseStatement();
            if (statement)
            {
                block->addStatement(std::move(statement));
            }
        }

        tokenStream.expect(TokenType::RBRACE);
        return std::move(block);
    }

    std::unique_ptr<ASTNode> StatementParser::delegateToSpecializedParser(StatementType type)
    {
        try
        {
            switch (type)
            {
            case StatementType::CONTROL_FLOW:
                {
                    switch (tokenStream.current().type)
                    {
                    case TokenType::IF: return parseIfStatement();
                    case TokenType::SWITCH: return parseSwitchStatement();
                    case TokenType::MATCH: return parseMatchStatement();
                    case TokenType::BREAK: return parseBreakStatement();
                    case TokenType::CONTINUE: return parseContinueStatement();
                    case TokenType::RETURN: return parseReturnStatement();
                    default:
                        throw ParseException("Invalid control flow token", tokenStream.current().location);
                    }
                }
            case StatementType::LOOP:
                {
                    switch (tokenStream.current().type)
                    {
                    case TokenType::WHILE: return parseWhileStatement();
                    case TokenType::DO: return parseDoWhileStatement();
                    case TokenType::FOR: return parseForStatement();
                    default:
                        throw ParseException("Invalid loop token", tokenStream.current().location);
                    }
                }
            case StatementType::DECLARATION:
                return parseDeclaration();
            case StatementType::ASSIGNMENT:
                return parseAssignment();
            case StatementType::FUNCTION:
                return parseFunction();
            case StatementType::IMPORT:
                return parseImport();
            case StatementType::EXCEPTION:
                {
                    switch (tokenStream.current().type)
                    {
                    case TokenType::TRY: return parseTryStatement();
                    case TokenType::THROW: return parseThrowStatement();
                    default:
                        throw ParseException("Invalid exception handling token", tokenStream.current().location);
                    }
                }
            case StatementType::EXPRESSION:
                return parseExpressionStatement();
            default:
                throw ParseException("Unknown statement type", tokenStream.current().location);
            }
        }
        catch (const ParseException&)
        {
            throw;
        }
        catch (const std::exception&)
        {
            throw ParseException("Parser error during statement processing", tokenStream.current().location);
        }
    }

    // ============================================================
    // Control flow (absorbed from ControlFlowParser)
    // ============================================================

    std::unique_ptr<ASTNode> StatementParser::parseIfStatement()
    {
        expectToken(TokenType::IF);
        expectToken(TokenType::LPAREN);
        auto condition = context.parseExpression();
        expectToken(TokenType::RPAREN);

        auto thenStatement = context.parseStatement();

        std::unique_ptr<ASTNode> elseStatement = nullptr;
        if (tryConsumeToken(TokenType::ELSE))
        {
            elseStatement = context.parseStatement();
        }

        return std::make_unique<IfNode>(std::move(condition), std::move(thenStatement), std::move(elseStatement));
    }

    std::unique_ptr<ASTNode> StatementParser::parseSwitchStatement()
    {
        expectToken(TokenType::SWITCH);
        expectToken(TokenType::LPAREN);
        auto expression = context.parseExpression();
        expectToken(TokenType::RPAREN);
        expectToken(TokenType::LBRACE);

        auto switchNode = std::make_unique<SwitchNode>(std::move(expression));

        while (!tokenStream.check(TokenType::RBRACE) && !isAtEnd())
        {
            if (tokenStream.check(TokenType::CASE))
            {
                tokenStream.advance();
                auto caseValue = context.parseExpression();
                expectToken(TokenType::COLON);

                auto caseNode = std::make_unique<CaseNode>(std::move(caseValue));
                while (!tokenStream.check(TokenType::CASE) &&
                    !tokenStream.check(TokenType::DEFAULT) &&
                    !tokenStream.check(TokenType::RBRACE) &&
                    !isAtEnd())
                {
                    auto stmt = context.parseStatement();
                    if (stmt)
                    {
                        caseNode->addStatement(std::move(stmt));
                    }
                }

                switchNode->addCase(std::move(caseNode));
            }
            else if (tokenStream.check(TokenType::DEFAULT))
            {
                auto defaultLocation = tokenStream.current().location;
                tokenStream.advance();
                expectToken(TokenType::COLON);

                auto defaultNode = std::make_unique<DefaultCaseNode>(defaultLocation);
                while (!tokenStream.check(TokenType::CASE) &&
                    !tokenStream.check(TokenType::DEFAULT) &&
                    !tokenStream.check(TokenType::RBRACE) &&
                    !isAtEnd())
                {
                    auto stmt = context.parseStatement();
                    if (stmt)
                    {
                        defaultNode->addStatement(std::move(stmt));
                    }
                }

                switchNode->addCase(std::move(defaultNode));
            }
            else
            {
                tokenStream.advance();
            }
        }

        expectToken(TokenType::RBRACE);
        return std::move(switchNode);
    }

    std::unique_ptr<ASTNode> StatementParser::parseMatchStatement()
    {
        auto matchLocation = tokenStream.current().location;
        expectToken(TokenType::MATCH);
        expectToken(TokenType::LPAREN);
        auto expression = context.parseExpression();
        expectToken(TokenType::RPAREN);
        expectToken(TokenType::LBRACE);

        auto matchNode = std::make_unique<MatchNode>(std::move(expression), matchLocation);

        while (!tokenStream.check(TokenType::RBRACE) && !isAtEnd())
        {
            if (tokenStream.check(TokenType::DEFAULT))
            {
                auto defaultLoc = tokenStream.current().location;
                tokenStream.advance();
                expectToken(TokenType::ARROW);

                std::unique_ptr<ASTNode> body;
                if (tokenStream.check(TokenType::LBRACE))
                {
                    body = context.parseStatement();
                }
                else
                {
                    body = context.parseStatement();
                }

                matchNode->addCase(std::make_unique<MatchDefaultNode>(std::move(body), defaultLoc));
            }
            else if (tokenStream.check(TokenType::CASE))
            {
                auto caseLoc = tokenStream.current().location;
                tokenStream.advance();

                if (tokenStream.check(TokenType::NULL_LITERAL))
                {
                    tokenStream.advance();
                    expectToken(TokenType::ARROW);

                    std::unique_ptr<ASTNode> body;
                    if (tokenStream.check(TokenType::LBRACE))
                    {
                        body = context.parseStatement();
                    }
                    else
                    {
                        body = context.parseStatement();
                    }

                    matchNode->addCase(std::make_unique<MatchCaseNode>(
                        std::move(body), PatternKind::NULL_PATTERN, caseLoc));
                }
                else if (isTypePatternStart())
                {
                    std::string typeName = std::string(tokenStream.current().stringValue);
                    tokenStream.advance();

                    std::string bindingName = std::string(tokenStream.current().stringValue);
                    expectToken(TokenType::IDENTIFIER);
                    expectToken(TokenType::ARROW);

                    std::unique_ptr<ASTNode> body;
                    if (tokenStream.check(TokenType::LBRACE))
                    {
                        body = context.parseStatement();
                    }
                    else
                    {
                        body = context.parseStatement();
                    }

                    matchNode->addCase(std::make_unique<MatchCaseNode>(
                        typeName, bindingName, std::move(body), caseLoc));
                }
                else
                {
                    auto valueExpr = context.parseExpression();
                    expectToken(TokenType::ARROW);

                    std::unique_ptr<ASTNode> body;
                    if (tokenStream.check(TokenType::LBRACE))
                    {
                        body = context.parseStatement();
                    }
                    else
                    {
                        body = context.parseStatement();
                    }

                    matchNode->addCase(std::make_unique<MatchCaseNode>(
                        std::move(valueExpr), std::move(body), caseLoc));
                }
            }
            else
            {
                throw ParseException("Expected 'case' or 'default' in match statement",
                                     tokenStream.current().location);
            }
        }

        expectToken(TokenType::RBRACE);
        return std::move(matchNode);
    }

    std::unique_ptr<ASTNode> StatementParser::parseBreakStatement()
    {
        auto breakLocation = tokenStream.current().location;
        expectToken(TokenType::BREAK);
        expectToken(TokenType::SEMICOLON);
        return std::make_unique<BreakNode>(breakLocation);
    }

    std::unique_ptr<ASTNode> StatementParser::parseContinueStatement()
    {
        expectToken(TokenType::CONTINUE);
        expectToken(TokenType::SEMICOLON);
        return std::make_unique<ContinueNode>(tokenStream.current().location);
    }

    std::unique_ptr<ASTNode> StatementParser::parseReturnStatement()
    {
        const auto returnLocation = tokenStream.current().location;
        expectToken(TokenType::RETURN);

        std::unique_ptr<ASTNode> value = nullptr;
        if (!tokenStream.check(TokenType::SEMICOLON))
        {
            value = context.parseExpression();
        }

        expectToken(TokenType::SEMICOLON);
        return std::make_unique<ReturnNode>(std::move(value), returnLocation);
    }

    bool StatementParser::isTypePatternStart() const
    {
        auto currentType = tokenStream.current().type;

        bool isTypeKeyword = (currentType == TokenType::INT ||
                              currentType == TokenType::FLOAT ||
                              currentType == TokenType::BOOL ||
                              currentType == TokenType::STRING_TYPE);

        bool isClassName = (currentType == TokenType::IDENTIFIER &&
                            !tokenStream.current().stringValue.empty() &&
                            std::isupper(tokenStream.current().stringValue[0]));

        if (!isTypeKeyword && !isClassName) return false;

        auto nextToken = tokenStream.peekAhead(1);
        return nextToken.type == TokenType::IDENTIFIER;
    }

    // ============================================================
    // Loops (absorbed from LoopParser)
    // ============================================================

    std::unique_ptr<ASTNode> StatementParser::parseWhileStatement()
    {
        expectToken(TokenType::WHILE);
        expectToken(TokenType::LPAREN);
        auto condition = context.parseExpression();
        expectToken(TokenType::RPAREN);

        auto body = context.parseStatement();
        return std::make_unique<WhileNode>(std::move(condition), std::move(body));
    }

    std::unique_ptr<ASTNode> StatementParser::parseDoWhileStatement()
    {
        expectToken(TokenType::DO);
        auto body = context.parseStatement();
        expectToken(TokenType::WHILE);
        expectToken(TokenType::LPAREN);
        auto condition = context.parseExpression();
        expectToken(TokenType::RPAREN);
        expectToken(TokenType::SEMICOLON);

        return std::make_unique<DoWhileNode>(std::move(body), std::move(condition));
    }

    std::unique_ptr<ASTNode> StatementParser::parseForStatement()
    {
        expectToken(TokenType::FOR);
        expectToken(TokenType::LPAREN);

        std::unique_ptr<ASTNode> init = nullptr;
        if (!tokenStream.check(TokenType::SEMICOLON))
        {
            if (tokenStream.check(TokenType::INT) ||
                tokenStream.check(TokenType::FLOAT) ||
                tokenStream.check(TokenType::BOOL) ||
                tokenStream.check(TokenType::STRING_TYPE) ||
                tokenStream.check(TokenType::IDENTIFIER))
            {
                TypeInfo typeInfo = TypeParser::parseTypeInfo(tokenStream);

                if (!tokenStream.check(TokenType::IDENTIFIER))
                {
                    throw ParseException("Expected variable name", tokenStream.current().location);
                }

                std::string varName = std::string(tokenStream.current().stringValue);
                SourceLocation location = tokenStream.current().location;
                tokenStream.advance();

                if (tokenStream.check(TokenType::COLON))
                {
                    tokenStream.advance();

                    auto collection = context.parseExpression();
                    expectToken(TokenType::RPAREN);

                    auto body = context.parseStatement();

                    return std::make_unique<ForEachNode>(varName, typeInfo,
                                                         std::move(collection), std::move(body), location);
                }
                else
                {
                    std::unique_ptr<ASTNode> value = nullptr;
                    if (tryConsumeToken(TokenType::ASSIGN))
                    {
                        value = context.parseExpression();
                    }

                    init = std::make_unique<AssignmentNode>(varName, std::move(value),
                                                            typeInfo.baseType, typeInfo.className,
                                                            false, false, location);
                }
            }
            else
            {
                init = context.parseExpression();
            }
        }

        expectToken(TokenType::SEMICOLON);

        std::unique_ptr<ASTNode> condition = nullptr;
        if (!tokenStream.check(TokenType::SEMICOLON))
        {
            condition = context.parseExpression();
        }
        expectToken(TokenType::SEMICOLON);

        std::unique_ptr<ASTNode> update = nullptr;
        if (!tokenStream.check(TokenType::RPAREN))
        {
            update = context.parseExpression();
        }
        expectToken(TokenType::RPAREN);

        auto body = context.parseStatement();

        return std::make_unique<ForNode>(std::move(init), std::move(condition),
                                         std::move(update), std::move(body));
    }

    // ============================================================
    // Declaration (absorbed from DeclarationParser)
    // ============================================================

    std::unique_ptr<ASTNode> StatementParser::parseDeclaration()
    {
        VisibilityModifier visibility = VisibilityParser::parseVisibilityModifier(tokenStream);

        ModifierInfo modifiers = parseModifiers();

        auto typeLocation = tokenStream.current().location;

        TypeInfo typeInfo;
        typeInfo = TypeParser::parseTypeInfo(tokenStream);
        ValueType type = typeInfo.baseType;
        std::string className = typeInfo.className;
        bool isNullableType = typeInfo.isNullable;

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            if (tokenStream.check(TokenType::LPAREN) && !className.empty() &&
                className.find("::") != std::string::npos)
            {
                tokenStream.advance();
                std::vector<std::unique_ptr<ASTNode>> arguments;

                if (!tokenStream.check(TokenType::RPAREN))
                {
                    arguments.push_back(context.parseExpression());
                    while (tryConsumeToken(TokenType::COMMA))
                    {
                        arguments.push_back(context.parseExpression());
                    }
                }

                expectToken(TokenType::RPAREN);
                expectToken(TokenType::SEMICOLON);

                return std::make_unique<FunctionCallNode>(className, std::move(arguments), typeLocation);
            }

            throw ParseException("Expected variable name", tokenStream.current().location);
        }

        std::string varName = std::string(tokenStream.current().stringValue);
        SourceLocation varLocation = tokenStream.current().location;

        NameValidator::validateIdentifierName(varName, "Variable", varLocation);

        tokenStream.advance();

        std::unique_ptr<ASTNode> value = nullptr;
        if (tryConsumeToken(TokenType::ASSIGN))
        {
            value = context.parseExpression();

            value = applyAutoBoxingIfNeeded(std::move(value), className, type);
        }

        expectToken(TokenType::SEMICOLON);

        auto assignmentNode = std::make_unique<AssignmentNode>(varName, std::move(value), type, className,
                                                               modifiers.isFinal, modifiers.isStatic, varLocation);
        assignmentNode->setVisibility(visibility);
        assignmentNode->setNullableType(isNullableType);
        return assignmentNode;
    }

    StatementParser::ModifierInfo StatementParser::parseModifiers()
    {
        ModifierInfo modifiers;

        if (tryConsumeToken(TokenType::FINAL))
        {
            modifiers.isFinal = true;
        }

        if (tryConsumeToken(TokenType::STATIC))
        {
            modifiers.isStatic = true;
        }

        return modifiers;
    }

    std::unique_ptr<ASTNode> StatementParser::applyAutoBoxingIfNeeded(
        std::unique_ptr<ASTNode> value,
        const std::string& targetClassName,
        value::ValueType targetType)
    {
        if (!value)
        {
            return value;
        }

        if (targetClassName.length() > 2 && targetClassName.substr(targetClassName.length() - 2) == "[]")
        {
            std::string elementType = targetClassName.substr(0, targetClassName.length() - 2);

            bool isBoxArrayType = (elementType == "Int" ||
                                   elementType == "Float" ||
                                   elementType == "Bool" ||
                                   elementType == "String");

            if (isBoxArrayType)
            {
                if (auto* arrayLiteral = dynamic_cast<ArrayLiteralNode*>(value.get()))
                {
                    auto& elements = const_cast<std::vector<std::unique_ptr<ASTNode>>&>(arrayLiteral->getElements());

                    for (size_t i = 0; i < elements.size(); ++i)
                    {
                        ASTNode* elem = elements[i].get();
                        bool shouldBox = false;

                        if (elementType == "Int" && dynamic_cast<IntegerNode*>(elem))
                        {
                            shouldBox = true;
                        }
                        else if (elementType == "Float" && dynamic_cast<FloatNode*>(elem))
                        {
                            shouldBox = true;
                        }
                        else if (elementType == "Bool" && dynamic_cast<BoolNode*>(elem))
                        {
                            shouldBox = true;
                        }
                        else if (elementType == "String" && dynamic_cast<StringNode*>(elem))
                        {
                            shouldBox = true;
                        }

                        if (shouldBox)
                        {
                            SourceLocation loc = elem->getLocation();

                            std::vector<std::unique_ptr<ASTNode>> constructorArgs;
                            constructorArgs.push_back(std::move(elements[i]));

                            elements[i] = std::make_unique<NewNode>(
                                elementType,
                                std::move(constructorArgs),
                                loc
                            );
                        }
                    }

                    return value;
                }
            }
        }

        if (targetType != value::ValueType::OBJECT)
        {
            return value;
        }

        bool isBoxType = (targetClassName == "Int" ||
                          targetClassName == "Float" ||
                          targetClassName == "Bool" ||
                          targetClassName == "String");

        if (!isBoxType)
        {
            return value;
        }

        bool needsBoxing = false;

        if (targetClassName == "Int" && dynamic_cast<IntegerNode*>(value.get()))
        {
            needsBoxing = true;
        }
        else if (targetClassName == "Float" && dynamic_cast<FloatNode*>(value.get()))
        {
            needsBoxing = true;
        }
        else if (targetClassName == "Bool" && dynamic_cast<BoolNode*>(value.get()))
        {
            needsBoxing = true;
        }
        else if (targetClassName == "String" && dynamic_cast<StringNode*>(value.get()))
        {
            needsBoxing = true;
        }

        if (!needsBoxing)
        {
            return value;
        }

        SourceLocation loc = value->getLocation();

        std::vector<std::unique_ptr<ASTNode>> constructorArgs;
        constructorArgs.push_back(std::move(value));

        auto boxedValue = std::make_unique<NewNode>(
            targetClassName,
            std::move(constructorArgs),
            loc
        );

        return boxedValue;
    }

    // ============================================================
    // Assignment (absorbed from AssignmentStatementParser)
    // ============================================================

    std::unique_ptr<ASTNode> StatementParser::parseAssignment()
    {
        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException("Expected identifier in assignment", tokenStream.current().location);
        }

        std::string varName = std::string(tokenStream.current().stringValue);
        SourceLocation varLocation = tokenStream.location();
        tokenStream.advance();

        TokenType opType = tokenStream.current().type;
        if (isAssignmentOperator(opType))
        {
            tokenStream.advance();
            if (!expressionParser)
            {
                throw ParseException("ExpressionParser not initialized", tokenStream.current().location);
            }
            auto value = expressionParser->parseExpression();

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

    std::unique_ptr<ASTNode> StatementParser::parseExpressionStatement()
    {
        if (!expressionParser)
        {
            throw ParseException("ExpressionParser not initialized", tokenStream.current().location);
        }
        auto expr = expressionParser->parseExpression();

        if (auto memberAccess = dynamic_cast<MemberAccessNode*>(expr.get()))
        {
            TokenType opType = tokenStream.current().type;
            if (isAssignmentOperator(opType))
            {
                errors::SourceLocation opLocation = tokenStream.current().location;
                tokenStream.advance();
                auto value = expressionParser->parseExpression();

                if (opType != TokenType::ASSIGN)
                {
                    TokenType binaryOp = getCorrespondingBinaryOperator(opType);
                    value = std::make_unique<BinaryExpNode>(std::move(expr), binaryOp, std::move(value), opLocation);

                    expectToken(TokenType::SEMICOLON);
                    return value;
                }

                expectToken(TokenType::SEMICOLON);

                auto object = memberAccess->getObjectShared();
                std::string memberName = memberAccess->getMemberName();

                return std::make_unique<MemberAssignmentNode>(object, memberName, std::move(value),
                                                              opLocation, memberAccess->getIsSafe());
            }
        }

        if (auto superMemberAccess = dynamic_cast<SuperMemberAccessNode*>(expr.get()))
        {
            TokenType opType = tokenStream.current().type;
            if (isAssignmentOperator(opType))
            {
                errors::SourceLocation opLocation = tokenStream.current().location;
                tokenStream.advance();
                auto value = expressionParser->parseExpression();

                if (opType != TokenType::ASSIGN)
                {
                    TokenType binaryOp = getCorrespondingBinaryOperator(opType);
                    value = std::make_unique<BinaryExpNode>(std::move(expr), binaryOp, std::move(value), opLocation);

                    expectToken(TokenType::SEMICOLON);
                    return value;
                }

                expectToken(TokenType::SEMICOLON);

                std::string memberName = superMemberAccess->getMemberName();

                return std::make_unique<SuperMemberAssignmentNode>(memberName, std::move(value), opLocation);
            }
        }

        if (auto indexAccess = dynamic_cast<IndexAccessNode*>(expr.get()))
        {
            TokenType opType = tokenStream.current().type;
            if (isAssignmentOperator(opType))
            {
                errors::SourceLocation opLocation = tokenStream.current().location;
                tokenStream.advance();
                auto value = expressionParser->parseExpression();

                if (opType != TokenType::ASSIGN)
                {
                    TokenType binaryOp = getCorrespondingBinaryOperator(opType);
                    value = std::make_unique<BinaryExpNode>(std::move(expr), binaryOp, std::move(value), opLocation);

                    expectToken(TokenType::SEMICOLON);
                    return value;
                }

                expectToken(TokenType::SEMICOLON);

                auto object = indexAccess->transferCollectionOwnership();
                auto index = indexAccess->transferIndexOwnership();

                return std::make_unique<IndexAssignmentNode>(std::move(object), std::move(index), std::move(value));
            }
        }

        expectToken(TokenType::SEMICOLON);
        return expr;
    }

    bool StatementParser::isAssignmentOperator(TokenType type) const noexcept
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

    TokenType StatementParser::getCorrespondingBinaryOperator(TokenType assignmentOp) const
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
            return TokenType::PLUS;
        }
    }

    std::unique_ptr<ASTNode> StatementParser::createCompoundAssignment(
        const std::string& varName,
        const SourceLocation& location,
        TokenType opType,
        std::unique_ptr<ASTNode> value)
    {
        auto varNode = std::make_unique<VariableNode>(varName, location);
        TokenType binaryOp = getCorrespondingBinaryOperator(opType);

        return std::make_unique<BinaryExpNode>(std::move(varNode), binaryOp, std::move(value), location);
    }

    // ============================================================
    // Function (absorbed from FunctionParser)
    // ============================================================

    std::unique_ptr<ASTNode> StatementParser::parseFunction()
    {
        auto annotations = AnnotationParser::parseAnnotations(tokenStream, &context);

        if (context.isInsideFunctionBody())
        {
            throw ParseException("Function declarations inside function bodies are not allowed. "
                                 "Functions must be declared at the top level or inside classes.",
                                 tokenStream.current().location);
        }

        VisibilityModifier visibility = VisibilityParser::parseVisibilityModifier(tokenStream);

        bool isAsync = false;

        expectToken(TokenType::FUNCTION);

        if (tryConsumeToken(TokenType::ASYNC))
        {
            isAsync = true;
        }

        std::vector<GenericTypeParameter> functionGenericParameters;
        if (tokenStream.check(TokenType::LESS))
        {
            tokenStream.advance();
            GenericParameterParser genericParser(tokenStream, context);
            functionGenericParameters = genericParser.parseGenericTypeParameters();
            tokenStream.expectGreaterForGeneric();
        }

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException("Expected function name", tokenStream.current().location);
        }

        std::string funcName = std::string(tokenStream.current().stringValue);
        auto funcLocation = tokenStream.current().location;
        validateFunctionName(funcName);
        tokenStream.advance();

        auto paramDecls = ParameterParser::parseGenericParameterDeclList(tokenStream, true);
        std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>> genericParameters;
        std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> parameterAnnotations;
        genericParameters.reserve(paramDecls.size());
        parameterAnnotations.reserve(paramDecls.size());
        for (auto& d : paramDecls) {
            genericParameters.emplace_back(d.name, d.type);
            parameterAnnotations.push_back(std::move(d.annotations));
        }

        expectToken(TokenType::COLON);

        auto genericReturnType = TypeParser::parseGenericType(tokenStream);

        if (isAsync)
        {
            AsyncValidator::validateAsyncReturnType(genericReturnType, tokenStream.location());
        }

        ParseContext::FunctionContextGuard functionGuard(context.getContextState());
        ParseContext::AsyncContextGuard asyncGuard(context.getContextState(), isAsync);
        std::unique_ptr<ASTNode> body = context.parseStatement();

        auto funcNode = std::make_unique<FunctionNode>(funcName, genericReturnType,
                                                       genericParameters, std::move(body),
                                                       functionGenericParameters, isAsync, funcLocation);
        funcNode->setVisibility(visibility);

        for (auto& annotation : annotations)
        {
            funcNode->addAnnotation(annotation);
        }

        funcNode->setParameterAnnotations(std::move(parameterAnnotations));

        return funcNode;
    }

    void StatementParser::validateFunctionName(const std::string& funcName)
    {
        NameValidator::validateFunctionNamingConvention(funcName, false, "Function", tokenStream.location());
    }

    // ============================================================
    // Import (absorbed from ImportParser)
    // ============================================================

    std::unique_ptr<ASTNode> StatementParser::parseImport()
    {
        SourceLocation loc = tokenStream.current().location;
        expectToken(TokenType::IMPORT);

        if (tokenStream.check(TokenType::MULTIPLY))
        {
            tokenStream.advance();
            return parseWildcardImport(loc);
        }

        if (tokenStream.check(TokenType::IDENTIFIER) &&
            tokenStream.current().stringValue == "lib")
        {
            tokenStream.advance();
            return parseLibraryImport(loc);
        }

        if (tokenStream.check(TokenType::LBRACE))
        {
            return parseSelectiveImport(loc);
        }

        throw ParseException(
            "Expected '{' for selective import, '*' for wildcard import, or 'lib' for library import after 'import' keyword. "
            "Syntax: 'import {Symbol} from \"file.mt\"', 'import * from \"file.mt\"', or 'import lib \"name\"'",
            tokenStream.current().location);
    }

    std::unique_ptr<ASTNode> StatementParser::parseSelectiveImport(const SourceLocation& loc)
    {
        expectToken(TokenType::LBRACE);

        std::vector<std::string> symbols;
        std::unordered_map<std::string, std::string> aliases;

        while (!tokenStream.check(TokenType::RBRACE))
        {
            if (!tokenStream.check(TokenType::IDENTIFIER))
            {
                throw ParseException("Expected identifier in import list", tokenStream.current().location);
            }

            std::string symbolName = std::string(tokenStream.current().stringValue);
            SourceLocation symbolLocation = tokenStream.current().location;

            NameValidator::validateIdentifierName(symbolName, "Import symbol", symbolLocation);

            symbols.push_back(symbolName);
            tokenStream.advance();

            if (tokenStream.check(TokenType::IDENTIFIER) &&
                tokenStream.current().stringValue == "as")
            {
                tokenStream.advance();

                if (!tokenStream.check(TokenType::IDENTIFIER))
                {
                    throw ParseException("Expected alias name after 'as'", tokenStream.current().location);
                }

                std::string aliasName = std::string(tokenStream.current().stringValue);
                NameValidator::validateIdentifierName(aliasName, "Import alias", tokenStream.current().location);
                aliases[symbolName] = aliasName;
                tokenStream.advance();
            }

            if (tokenStream.check(TokenType::COMMA))
            {
                tokenStream.advance();
            }
            else if (!tokenStream.check(TokenType::RBRACE))
            {
                throw ParseException("Expected ',', 'as', or '}' in import list", tokenStream.current().location);
            }
        }

        expectToken(TokenType::RBRACE);

        if (symbols.empty())
        {
            throw ParseException("Import list cannot be empty", loc);
        }

        expectToken(TokenType::FROM);

        if (!tokenStream.check(TokenType::STRING_LITERAL))
        {
            throw ParseException("Expected string literal after 'from'", tokenStream.current().location);
        }

        std::string filePath = std::string(tokenStream.current().stringValue);
        tokenStream.advance();

        validateImportPath(filePath);

        expectToken(TokenType::SEMICOLON);

        auto node = std::make_unique<ImportNode>(filePath, ImportType::SELECTIVE, symbols, loc);
        for (const auto& [original, alias] : aliases) {
            node->addSymbolAlias(original, alias);
        }
        return node;
    }

    std::unique_ptr<ASTNode> StatementParser::parseWildcardImport(const SourceLocation& loc)
    {
        expectToken(TokenType::FROM);

        if (!tokenStream.check(TokenType::STRING_LITERAL))
        {
            throw ParseException("Expected string literal after 'from'", tokenStream.current().location);
        }

        std::string filePath = std::string(tokenStream.current().stringValue);
        tokenStream.advance();

        validateImportPath(filePath);

        expectToken(TokenType::SEMICOLON);

        return std::make_unique<ImportNode>(filePath, ImportType::WILDCARD, loc);
    }

    std::unique_ptr<ASTNode> StatementParser::parseLibraryImport(const SourceLocation& loc)
    {
        if (tokenStream.check(TokenType::LBRACE))
        {
            expectToken(TokenType::LBRACE);

            std::vector<std::string> symbols;
            std::unordered_map<std::string, std::string> aliases;

            while (!tokenStream.check(TokenType::RBRACE))
            {
                if (!tokenStream.check(TokenType::IDENTIFIER))
                {
                    throw ParseException("Expected identifier in library import list", tokenStream.current().location);
                }

                std::string symbolName = std::string(tokenStream.current().stringValue);
                SourceLocation symbolLocation = tokenStream.current().location;
                NameValidator::validateIdentifierName(symbolName, "Library import symbol", symbolLocation);
                symbols.push_back(symbolName);
                tokenStream.advance();

                if (tokenStream.check(TokenType::IDENTIFIER) &&
                    tokenStream.current().stringValue == "as")
                {
                    tokenStream.advance();

                    if (!tokenStream.check(TokenType::IDENTIFIER))
                    {
                        throw ParseException("Expected alias name after 'as'", tokenStream.current().location);
                    }

                    std::string aliasName = std::string(tokenStream.current().stringValue);
                    NameValidator::validateIdentifierName(aliasName, "Import alias", tokenStream.current().location);
                    aliases[symbolName] = aliasName;
                    tokenStream.advance();
                }

                if (tokenStream.check(TokenType::COMMA))
                {
                    tokenStream.advance();
                }
                else if (!tokenStream.check(TokenType::RBRACE))
                {
                    throw ParseException("Expected ',', 'as', or '}' in library import list", tokenStream.current().location);
                }
            }

            expectToken(TokenType::RBRACE);

            if (symbols.empty())
            {
                throw ParseException("Library import list cannot be empty", loc);
            }

            expectToken(TokenType::FROM);

            if (!tokenStream.check(TokenType::STRING_LITERAL))
            {
                throw ParseException("Expected library name string after 'from'", tokenStream.current().location);
            }

            std::string libraryName = std::string(tokenStream.current().stringValue);
            tokenStream.advance();

            if (libraryName.empty())
            {
                throw ParseException("Library name cannot be empty", tokenStream.current().location);
            }

            expectToken(TokenType::SEMICOLON);

            auto node = std::make_unique<ImportNode>(libraryName, ImportType::LIBRARY_SELECTIVE, symbols, loc);
            node->setLibraryName(libraryName);
            for (const auto& [original, alias] : aliases) {
                node->addSymbolAlias(original, alias);
            }
            return node;
        }

        if (!tokenStream.check(TokenType::STRING_LITERAL))
        {
            throw ParseException("Expected library name string or '{' after 'lib'", tokenStream.current().location);
        }

        std::string libraryName = std::string(tokenStream.current().stringValue);
        tokenStream.advance();

        if (libraryName.empty())
        {
            throw ParseException("Library name cannot be empty", tokenStream.current().location);
        }

        expectToken(TokenType::SEMICOLON);

        auto node = std::make_unique<ImportNode>(libraryName, ImportType::LIBRARY, loc);
        node->setLibraryName(libraryName);
        return node;
    }

    void StatementParser::validateImportPath(const std::string& path)
    {
        if (path.empty())
        {
            throw ParseException("Import path cannot be empty", tokenStream.current().location);
        }
    }

    // ============================================================
    // Exception (absorbed from ExceptionParser)
    // ============================================================

    std::unique_ptr<TryNode> StatementParser::parseTryStatement()
    {
        auto tryLocation = tokenStream.current().location;
        expectToken(TokenType::TRY);

        auto tryBlock = context.parseStatement();

        std::vector<std::unique_ptr<CatchNode>> catchBlocks;
        std::unordered_set<std::string> seenCatchTypes;

        while (tokenStream.check(TokenType::CATCH))
        {
            auto catchBlock = parseCatchClause();
            const std::string& exceptionType = catchBlock->getExceptionType();

            if (seenCatchTypes.find(exceptionType) != seenCatchTypes.end())
            {
                throw ParseException(
                    "Duplicate catch block for exception type '" + exceptionType + "'",
                    tokenStream.current().location
                );
            }

            seenCatchTypes.insert(exceptionType);
            catchBlocks.push_back(std::move(catchBlock));
        }

        std::unique_ptr<ASTNode> finallyBlock = nullptr;
        if (tryConsumeToken(TokenType::FINALLY))
        {
            finallyBlock = parseFinallyClause();
        }

        if (catchBlocks.empty() && !finallyBlock)
        {
            throw ParseException("try statement must have at least one catch or finally clause", tryLocation);
        }

        return std::make_unique<TryNode>(
            std::move(tryBlock),
            std::move(catchBlocks),
            std::move(finallyBlock),
            tryLocation
        );
    }

    std::unique_ptr<CatchNode> StatementParser::parseCatchClause()
    {
        auto catchLocation = tokenStream.current().location;
        expectToken(TokenType::CATCH);
        expectToken(TokenType::LPAREN);

        auto exceptionType = TypeParser::parseGenericType(tokenStream);

        std::string exceptionTypeStr = exceptionType->toString();

        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            throw ParseException("Expected variable name in catch clause", tokenStream.current().location);
        }
        std::string variableName = std::string(tokenStream.current().stringValue);
        tokenStream.advance();

        expectToken(TokenType::RPAREN);

        auto body = context.parseStatement();

        return std::make_unique<CatchNode>(
            exceptionTypeStr,
            variableName,
            std::move(body),
            catchLocation
        );
    }

    std::unique_ptr<ASTNode> StatementParser::parseFinallyClause()
    {
        return context.parseStatement();
    }

    std::unique_ptr<ThrowNode> StatementParser::parseThrowStatement()
    {
        auto throwLocation = tokenStream.current().location;
        expectToken(TokenType::THROW);

        auto expression = context.parseExpression();

        expectToken(TokenType::SEMICOLON);

        return std::make_unique<ThrowNode>(std::move(expression), throwLocation);
    }
}
