#include "ClassParser.hpp"
#include "class/ClassDeclarationParser.hpp"
#include "class/ConstructorParser.hpp"
#include "class/MethodParser.hpp"
#include "class/FieldParser.hpp"
#include "class/ObjectCreationParser.hpp"
#include "class/GenericParameterParser.hpp"
#include "utilities/ParserUtils.hpp"
#include "../services/ImportManager.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../errors/ParseException.hpp"
#include <unordered_set>

namespace parser
{
    using namespace ast::nodes::classes;
    using namespace ast::nodes::expressions;
    using namespace token;
    using namespace value;
    using namespace errors;

    ClassParser::ClassParser(TokenStream& stream, ParseContext& ctx)
        : tokenStream(stream), context(ctx)
    {
        initializeHelperParsers();
    }

    ClassParser::~ClassParser() = default;

    void ClassParser::initializeHelperParsers()
    {
        classDeclarationParser = std::make_unique<ClassDeclarationParser>(tokenStream, context);
        constructorParser = std::make_unique<ConstructorParser>(tokenStream, context);
        methodParser = std::make_unique<MethodParser>(tokenStream, context);
        fieldParser = std::make_unique<FieldParser>(tokenStream, context);
        objectCreationParser = std::make_unique<ObjectCreationParser>(tokenStream, context);
        genericParameterParser = std::make_unique<GenericParameterParser>(tokenStream, context);
    }

    std::unique_ptr<ASTNode> ClassParser::parseClass()
    {
        // Validate: classes cannot be declared inside other classes or interfaces
        if (context.isInsideClassBody())
        {
            throw ParseException("Class declarations inside class bodies are not allowed. "
                               "Nested classes are not supported.",
                               tokenStream.current().location);
        }
        if (context.isInsideInterfaceBody())
        {
            throw ParseException("Class declarations inside interface bodies are not allowed.",
                               tokenStream.current().location);
        }

        // Delegate to ClassDeclarationParser for class header parsing
        auto classNode = classDeclarationParser->parseClassDeclaration();
        auto* classNodePtr = dynamic_cast<ClassNode*>(classNode.get());

        if (!classNodePtr)
        {
            throw ParseException("Failed to create class node", tokenStream.current().location);
        }

        // Check for duplicate class/interface name
        const std::string& className = classNodePtr->getClassName();
        if (context.isTypeDeclared(className))
        {
            throw ParseException(
                "Duplicate type declaration: '" + className + "' has already been declared as a class or interface",
                classNodePtr->getLocation()
            );
        }

        // Register the class name
        context.registerClass(className);

        // Track method signatures for this class (local to this function)
        std::unordered_set<std::string> declaredStaticMethodSignatures;
        std::unordered_set<std::string> declaredInstanceMethodSignatures;

        // Set class context when parsing class body
        ParseContext::ClassContextGuard classGuard(context);

        // Parse class body members
        while (tokenStream.current().type != TokenType::RBRACE && tokenStream.current().type != TokenType::END)
        {
            TokenType currentToken = tokenStream.current().type;

            // Check for constructor (with or without access modifier)
            if (currentToken == TokenType::CONSTRUCTOR ||
                ((currentToken == TokenType::PUBLIC || currentToken == TokenType::PRIVATE || currentToken == TokenType::PROTECTED) &&
                 tokenStream.peekAhead(1).type == TokenType::CONSTRUCTOR))
            {
                auto constructor = constructorParser->parseConstructor();
                if (constructor)
                {
                    classNodePtr->addConstructor(std::move(constructor));
                }
            }
            // Check for method (with access modifiers, static, or just function)
            else if (currentToken == TokenType::FUNCTION ||
                (currentToken == TokenType::STATIC && tokenStream.peekAhead(1).type == TokenType::FUNCTION) ||
                ((currentToken == TokenType::PUBLIC || currentToken == TokenType::PRIVATE || currentToken == TokenType::PROTECTED) &&
                 tokenStream.peekAhead(1).type == TokenType::FUNCTION) ||
                ((currentToken == TokenType::PUBLIC || currentToken == TokenType::PRIVATE || currentToken == TokenType::PROTECTED) &&
                 tokenStream.peekAhead(1).type == TokenType::STATIC && tokenStream.peekAhead(2).type == TokenType::FUNCTION))
            {
                auto method = methodParser->parseMethod();
                if (method)
                {
                    // Check for duplicate method signatures (static and instance tracked separately)
                    auto* methodNode = dynamic_cast<ast::nodes::classes::MethodNode*>(method.get());
                    if (methodNode)
                    {
                        const std::string& methodName = methodNode->getName();
                        bool isStatic = methodNode->getIsStatic();

                        // Build signature: "methodName(type1,type2,...)"
                        std::string signature = methodName + "(";
                        const auto& params = methodNode->getGenericParameters();
                        for (size_t i = 0; i < params.size(); ++i) {
                            if (i > 0) signature += ",";
                            // Use GenericType's toString() method for full type representation
                            signature += params[i].second->toString();
                        }
                        signature += ")";

                        if (isStatic)
                        {
                            // Check for duplicate static method signature
                            if (declaredStaticMethodSignatures.count(signature) > 0)
                            {
                                throw ParseException(
                                    "Duplicate static method declaration: '" + signature + "' has already been declared in class '" + className + "'",
                                    methodNode->getLocation()
                                );
                            }
                            declaredStaticMethodSignatures.insert(signature);
                        }
                        else
                        {
                            // Check for duplicate instance method signature
                            if (declaredInstanceMethodSignatures.count(signature) > 0)
                            {
                                throw ParseException(
                                    "Duplicate instance method declaration: '" + signature + "' has already been declared in class '" + className + "'",
                                    methodNode->getLocation()
                                );
                            }
                            declaredInstanceMethodSignatures.insert(signature);
                        }
                    }

                    classNodePtr->addMethod(std::move(method));
                }
            }
            else
            {
                // Default case - parse as field (handles static/final modifiers and static methods)
                auto field = fieldParser->parseField();
                if (field)
                {
                    // Check if the parsed field is actually a method (static function)
                    auto* methodNode = dynamic_cast<MethodNode*>(field.get());
                    if (methodNode)
                    {
                        // Check for duplicate method signatures
                        const std::string& methodName = methodNode->getName();
                        bool isStatic = methodNode->getIsStatic();

                        // Build signature: "methodName(type1,type2,...)"
                        std::string signature = methodName + "(";
                        const auto& params = methodNode->getGenericParameters();
                        for (size_t i = 0; i < params.size(); ++i) {
                            if (i > 0) signature += ",";
                            // Use GenericType's toString() method for full type representation
                            signature += params[i].second->toString();
                        }
                        signature += ")";

                        if (isStatic)
                        {
                            // Check for duplicate static method signature
                            if (declaredStaticMethodSignatures.count(signature) > 0)
                            {
                                throw ParseException(
                                    "Duplicate static method declaration: '" + signature + "' has already been declared in class '" + className + "'",
                                    methodNode->getLocation()
                                );
                            }
                            declaredStaticMethodSignatures.insert(signature);
                        }
                        else
                        {
                            // Check for duplicate instance method signature
                            if (declaredInstanceMethodSignatures.count(signature) > 0)
                            {
                                throw ParseException(
                                    "Duplicate instance method declaration: '" + signature + "' has already been declared in class '" + className + "'",
                                    methodNode->getLocation()
                                );
                            }
                            declaredInstanceMethodSignatures.insert(signature);
                        }

                        classNodePtr->addMethod(std::move(field));
                    }
                    else
                    {
                        classNodePtr->addField(std::move(field));
                    }
                }
            }
        }

        tokenStream.expect(TokenType::RBRACE);
        return classNode;
    }

    std::unique_ptr<ASTNode> ClassParser::parseConstructor()
    {
        return constructorParser->parseConstructor();
    }

    std::unique_ptr<ASTNode> ClassParser::parseMethod()
    {
        return methodParser->parseMethod();
    }

    std::unique_ptr<ASTNode> ClassParser::parseField()
    {
        return fieldParser->parseField();
    }

    std::unique_ptr<ASTNode> ClassParser::parseNewExpression()
    {
        return objectCreationParser->parseNewExpression();
    }

    std::string ClassParser::parseGenericParameters()
    {
        return genericParameterParser->parseGenericParameters();
    }

    std::string ClassParser::parseGenericParameter()
    {
        return genericParameterParser->parseGenericParameter();
    }

    std::vector<GenericTypeParameter> ClassParser::parseGenericTypeParameters()
    {
        return genericParameterParser->parseGenericTypeParameters();
    }

    GenericTypeParameter ClassParser::parseGenericTypeParameter()
    {
        return genericParameterParser->parseGenericTypeParameter();
    }
}
