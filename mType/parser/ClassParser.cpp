#include "ClassParser.hpp"
#include "ParserContextState.hpp"
#include "class/ClassDeclarationParser.hpp"
#include "class/ConstructorParser.hpp"
#include "class/MethodParser.hpp"
#include "class/FieldParser.hpp"
#include "class/ObjectCreationParser.hpp"
#include "class/GenericParameterParser.hpp"
#include "utilities/ParserUtils.hpp"
#include "utilities/AnnotationParser.hpp"
#include "../services/ImportManager.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../ast/nodes/classes/ConstructorNode.hpp"
#include "../ast/nodes/classes/FieldNode.hpp"
#include "../errors/ParseException.hpp"
#include "../errors/DuplicateDeclarationException.hpp"
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
        // Step 1: Parse annotations before class declaration
        auto annotations = utilities::AnnotationParser::parseAnnotations(tokenStream);

        // Step 2: Validate declaration context
        validateClassDeclarationContext();

        // Step 3: Parse and validate class header
        std::unique_ptr<ASTNode> classNode;
        auto* classNodePtr = parseAndValidateClassHeader(classNode);
        const std::string& className = classNodePtr->getClassName();

        // Step 4: Add annotations to class node
        for (auto& annotation : annotations)
        {
            classNodePtr->addAnnotation(annotation);
        }

        // Step 5: Track method signatures for duplicate detection
        std::unordered_set<std::string> declaredStaticMethodSignatures;
        std::unordered_set<std::string> declaredInstanceMethodSignatures;

        // Step 6: Set class context when parsing class body
        ParserContextState::ClassContextGuard classGuard(context.getContextState());

        // Step 7: Parse class body members
        parseClassMembers(classNodePtr, className,
                          declaredStaticMethodSignatures, declaredInstanceMethodSignatures);

        // Step 8: Expect closing brace
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

    void ClassParser::validateClassDeclarationContext()
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
    }

    ast::nodes::classes::ClassNode* ClassParser::parseAndValidateClassHeader(std::unique_ptr<ASTNode>& classNode)
    {
        // Delegate to ClassDeclarationParser for class header parsing
        classNode = classDeclarationParser->parseClassDeclaration();
        auto* classNodePtr = dynamic_cast<ClassNode*>(classNode.get());

        if (!classNodePtr)
        {
            throw ParseException("Failed to create class node", tokenStream.current().location);
        }

        // Check for duplicate class/interface name
        const std::string& className = classNodePtr->getClassName();
        const SourceLocation& classLocation = classNodePtr->getLocation();

        if (context.isTypeDeclared(className))
        {
            // Get the location of the first declaration for better error message
            SourceLocation firstLocation = context.getTypeDeclarationLocation(className);
            throw DuplicateDeclarationException(
                "class",
                className,
                firstLocation,
                classLocation
            );
        }

        // Register the class name with final modifier and location
        context.registerClass(className, classNodePtr->isFinal(), classLocation);

        return classNodePtr;
    }

    void ClassParser::parseClassMembers(
        ast::nodes::classes::ClassNode* classNodePtr,
        const std::string& className,
        std::unordered_set<std::string>& declaredStaticMethodSignatures,
        std::unordered_set<std::string>& declaredInstanceMethodSignatures)
    {
        // Parse class body members
        while (tokenStream.current().type != TokenType::RBRACE && tokenStream.current().type != TokenType::END)
        {
            // Parse annotations before each member (method, field, or constructor)
            auto annotations = utilities::AnnotationParser::parseAnnotations(tokenStream);

            TokenType currentToken = tokenStream.current().type;

            // Check for constructor (with or without access modifier)
            if (currentToken == TokenType::CONSTRUCTOR ||
                ((currentToken == TokenType::PUBLIC || currentToken == TokenType::PRIVATE || currentToken ==
                        TokenType::PROTECTED) &&
                    tokenStream.peekAhead(1).type == TokenType::CONSTRUCTOR))
            {
                auto constructor = constructorParser->parseConstructor();
                if (constructor)
                {
                    // MYT-108: attach pending annotations to the constructor.
                    if (auto* ctorNode = dynamic_cast<ast::nodes::classes::ConstructorNode*>(constructor.get()))
                    {
                        for (auto& annotation : annotations)
                        {
                            ctorNode->addAnnotation(annotation);
                        }
                    }
                    classNodePtr->addConstructor(std::move(constructor));
                }
            }
            // Check for method (with access modifiers, static, final, or just function)
            // Covers all patterns: function, static function, access function, access static function,
            // final function, static final function, access final function, access static final function
            else if (isMethodDeclaration(currentToken))
            {
                auto method = methodParser->parseMethod();
                if (method)
                {
                    // Check for duplicate method signatures (static and instance tracked separately)
                    auto* methodNode = dynamic_cast<ast::nodes::classes::MethodNode*>(method.get());
                    if (methodNode)
                    {
                        // Add annotations to method
                        for (auto& annotation : annotations)
                        {
                            methodNode->addAnnotation(annotation);
                        }

                        validateAndRegisterMethodSignature(methodNode, className,
                                                           declaredStaticMethodSignatures,
                                                           declaredInstanceMethodSignatures);
                    }

                    classNodePtr->addMethod(std::move(method));
                }
            }
            else
            {
                // Default case - parse as field (handles static/final modifiers)
                auto field = fieldParser->parseField();
                if (field)
                {
                    // MYT-108: attach pending annotations to the field.
                    if (auto* fieldNode = dynamic_cast<ast::nodes::classes::FieldNode*>(field.get()))
                    {
                        for (auto& annotation : annotations)
                        {
                            fieldNode->addAnnotation(annotation);
                        }
                    }
                    classNodePtr->addField(std::move(field));
                }
            }
        }
    }

    std::string ClassParser::buildMethodSignature(const ast::nodes::classes::MethodNode* methodNode) const
    {
        const std::string& methodName = methodNode->getName();

        // Build signature: "methodName(type1,type2,...)"
        std::string signature = methodName + "(";
        const auto& params = methodNode->getGenericParameters();
        for (size_t i = 0; i < params.size(); ++i)
        {
            if (i > 0) signature += ",";
            // Use GenericType's toString() method for full type representation
            signature += params[i].second->toString();
        }
        signature += ")";

        return signature;
    }

    void ClassParser::validateAndRegisterMethodSignature(
        ast::nodes::classes::MethodNode* methodNode,
        const std::string& className,
        std::unordered_set<std::string>& declaredStaticMethodSignatures,
        std::unordered_set<std::string>& declaredInstanceMethodSignatures)
    {
        bool isStatic = methodNode->getIsStatic();
        std::string signature = buildMethodSignature(methodNode);

        if (isStatic)
        {
            // Check for duplicate static method signature
            if (declaredStaticMethodSignatures.count(signature) > 0)
            {
                throw ParseException(
                    "Duplicate static method declaration: '" + signature +
                    "' has already been declared in class '" + className + "'",
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
                    "Duplicate instance method declaration: '" + signature +
                    "' has already been declared in class '" + className + "'",
                    methodNode->getLocation()
                );
            }
            declaredInstanceMethodSignatures.insert(signature);
        }
    }

    bool ClassParser::isMethodDeclaration(token::TokenType currentToken) const
    {
        using namespace token;

        // Direct function keyword
        if (currentToken == TokenType::FUNCTION)
        {
            return true;
        }

        // static [final] function
        if (currentToken == TokenType::STATIC)
        {
            TokenType next = tokenStream.peekAhead(1).type;
            if (next == TokenType::FUNCTION)
            {
                return true;
            }
            if (next == TokenType::FINAL && tokenStream.peekAhead(2).type == TokenType::FUNCTION)
            {
                return true;
            }
        }

        // final [static] function
        if (currentToken == TokenType::FINAL)
        {
            TokenType next = tokenStream.peekAhead(1).type;
            if (next == TokenType::FUNCTION)
            {
                return true;
            }
            if (next == TokenType::STATIC && tokenStream.peekAhead(2).type == TokenType::FUNCTION)
            {
                return true;
            }
        }

        // abstract function
        if (currentToken == TokenType::ABSTRACT)
        {
            TokenType next = tokenStream.peekAhead(1).type;
            if (next == TokenType::FUNCTION)
            {
                return true;
            }
        }

        // access_modifier [static] [final] [abstract] function
        if (currentToken == TokenType::PUBLIC || currentToken == TokenType::PRIVATE || currentToken ==
            TokenType::PROTECTED)
        {
            TokenType next1 = tokenStream.peekAhead(1).type;

            // access function
            if (next1 == TokenType::FUNCTION)
            {
                return true;
            }

            // access static [final] function
            if (next1 == TokenType::STATIC)
            {
                TokenType next2 = tokenStream.peekAhead(2).type;
                if (next2 == TokenType::FUNCTION)
                {
                    return true;
                }
                if (next2 == TokenType::FINAL && tokenStream.peekAhead(3).type == TokenType::FUNCTION)
                {
                    return true;
                }
            }

            // access final [static] function
            if (next1 == TokenType::FINAL)
            {
                TokenType next2 = tokenStream.peekAhead(2).type;
                if (next2 == TokenType::FUNCTION)
                {
                    return true;
                }
                if (next2 == TokenType::STATIC && tokenStream.peekAhead(3).type == TokenType::FUNCTION)
                {
                    return true;
                }
            }

            // access abstract function
            if (next1 == TokenType::ABSTRACT)
            {
                TokenType next2 = tokenStream.peekAhead(2).type;
                if (next2 == TokenType::FUNCTION)
                {
                    return true;
                }
            }
        }

        return false;
    }
}
