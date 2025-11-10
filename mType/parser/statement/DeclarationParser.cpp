#include "DeclarationParser.hpp"
#include "../TypeParser.hpp"
#include "../utilities/VisibilityParser.hpp"
#include "../utilities/ParserUtils.hpp"
#include "../../ast/nodes/statements/AssignmentNode.hpp"
#include "../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../ast/nodes/classes/NewNode.hpp"
#include "../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../ast/nodes/expressions/FloatNode.hpp"
#include "../../ast/nodes/expressions/BoolNode.hpp"
#include "../../ast/nodes/expressions/StringNode.hpp"
#include "../../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../errors/ParseException.hpp"

namespace parser::statement
{
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    using namespace token;
    using namespace errors;
    using namespace parser::utilities;

    DeclarationParser::DeclarationParser(TokenStream& stream, ParseContext& ctx)
        : BaseParser(stream, ctx)
    {
    }

    std::unique_ptr<ASTNode> DeclarationParser::parse()
    {
        return parseDeclaration();
    }

    bool DeclarationParser::canParse(const TokenStream& stream) const
    {
        return isDeclarationStart(stream.current().type);
    }

    std::unique_ptr<ASTNode> DeclarationParser::parseDeclaration()
    {
        // Parse visibility modifier (public/private) - default is PUBLIC
        VisibilityModifier visibility = VisibilityParser::parseVisibilityModifier(tokenStream);

        // Parse modifiers (final, static)
        ModifierInfo modifiers = parseModifiers();

        // Parse the complete type information
        auto typeLocation = tokenStream.current().location;

        TypeInfo typeInfo;
        try
        {
            typeInfo = TypeParser::parseTypeInfo(tokenStream);
        }
        catch (const std::exception&)
        {
            throw;
        }
        ValueType type = typeInfo.baseType;
        std::string className = typeInfo.className;


        if (!tokenStream.check(TokenType::IDENTIFIER))
        {
            // Special case: if we see a parenthesis after a qualified name,
            // it's likely a static method call that was mistakenly routed here
            if (tokenStream.check(TokenType::LPAREN) && !className.empty() &&
                className.find("::") != std::string::npos)
            {
                // This is actually a static method call (e.g., Class::method())
                // Parse it as a function call instead of a declaration

                tokenStream.advance(); // consume '('
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

                // Create a FunctionCallNode with the qualified name
                return std::make_unique<FunctionCallNode>(className, std::move(arguments), typeLocation);
            }

            throw ParseException("Expected variable name", tokenStream.current().location);
        }


        std::string varName;
        try
        {
            varName = tokenStream.current().stringValue.getString();
        }
        catch (const std::exception&)
        {
            throw;
        }

        SourceLocation varLocation = tokenStream.current().location;

        // Validate variable name contains no special characters
        ParserUtils::validateIdentifierName(varName, "Variable", varLocation);

        tokenStream.advance();

        std::unique_ptr<ASTNode> value = nullptr;
        if (tryConsumeToken(TokenType::ASSIGN))
        {
            value = context.parseExpression();

            // PHASE 4: Apply auto-boxing if needed
            // Transforms: Int x = 42; → Int x = new Int(42);
            value = applyAutoBoxingIfNeeded(std::move(value), className, type);
        }
        else
        {
        }

        expectToken(TokenType::SEMICOLON);

        auto assignmentNode = std::make_unique<AssignmentNode>(varName, std::move(value), type, className,
                                                               modifiers.isFinal, modifiers.isStatic, varLocation);
        assignmentNode->setVisibility(visibility);
        return assignmentNode;
    }

    bool DeclarationParser::isDeclarationStart(TokenType type) const noexcept
    {
        return isModifierToken(type) || isTypeToken(type) || type == TokenType::IDENTIFIER;
    }

    bool DeclarationParser::isModifierToken(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::FINAL:
        case TokenType::STATIC:
        case TokenType::PUBLIC:
        case TokenType::PRIVATE:
            return true;
        default:
            return false;
        }
    }

    bool DeclarationParser::isTypeToken(TokenType type) const noexcept
    {
        switch (type)
        {
        case TokenType::INT:
        case TokenType::FLOAT:
        case TokenType::BOOL:
        case TokenType::STRING_TYPE:
        case TokenType::VOID:
            return true;
        default:
            return false;
        }
    }

    DeclarationParser::ModifierInfo DeclarationParser::parseModifiers()
    {
        ModifierInfo modifiers;

        // Handle final modifier
        if (tryConsumeToken(TokenType::FINAL))
        {
            modifiers.isFinal = true;
        }

        // Handle static modifier
        if (tryConsumeToken(TokenType::STATIC))
        {
            modifiers.isStatic = true;
        }

        return modifiers;
    }

    // Phase 4: Auto-boxing implementation
    std::unique_ptr<ASTNode> DeclarationParser::applyAutoBoxingIfNeeded(
        std::unique_ptr<ASTNode> value,
        const std::string& targetClassName,
        value::ValueType targetType)
    {
        using namespace ast::nodes::expressions;
        using namespace ast::nodes::classes;

        // Only auto-box if we have a value
        if (!value)
        {
            return value;
        }

        // PHASE 4.5: Array initializer auto-boxing
        // Check if target is an array type (e.g., "Int[]", "Float[]")
        if (targetClassName.length() > 2 && targetClassName.substr(targetClassName.length() - 2) == "[]")
        {
            // Extract element type (e.g., "Int" from "Int[]")
            std::string elementType = targetClassName.substr(0, targetClassName.length() - 2);

            // Check if it's a Box type array
            bool isBoxArrayType = (elementType == "Int" ||
                                   elementType == "Float" ||
                                   elementType == "Bool" ||
                                   elementType == "String");

            if (isBoxArrayType)
            {
                // Check if value is an array literal
                if (auto* arrayLiteral = dynamic_cast<ArrayLiteralNode*>(value.get()))
                {
                    // Transform each primitive literal element to a NewNode
                    auto& elements = const_cast<std::vector<std::unique_ptr<ASTNode>>&>(arrayLiteral->getElements());

                    for (size_t i = 0; i < elements.size(); ++i)
                    {
                        ASTNode* elem = elements[i].get();
                        bool shouldBox = false;

                        // Check if element is a primitive literal matching the element type
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
                            // Transform: Int[] arr = [42] → Int[] arr = [new Int(42)]
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

                    return value;  // Return modified array literal
                }
            }
        }

        // Regular auto-boxing for single values (not arrays)
        if (targetType != value::ValueType::OBJECT)
        {
            return value;
        }

        // Check if target is a primitive Box type
        bool isBoxType = (targetClassName == "Int" ||
                          targetClassName == "Float" ||
                          targetClassName == "Bool" ||
                          targetClassName == "String");

        if (!isBoxType)
        {
            return value;  // Not a Box type, no auto-boxing
        }

        // Check if value is a primitive literal that needs boxing
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
            return value;  // Value is not a primitive literal, no auto-boxing needed
        }

        // PHASE 4 AUTO-BOXING: Transform literal to constructor call
        // Example: Int x = 42; → Int x = new Int(42);

        // Save location before moving value
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
}
