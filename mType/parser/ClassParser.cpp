#include "ClassParser.hpp"
#include <cstddef>
#include <utility>
#include "ParserContextState.hpp"
#include "ParserConstants.hpp"
#include "ParserValidator.hpp"
#include "TypeParser.hpp"
#include "class/GenericParameterParser.hpp"
#include "utilities/NameValidator.hpp"
#include "utilities/ParserUtils.hpp"
#include "utilities/AnnotationParser.hpp"
#include "utilities/ParameterParser.hpp"
#include "utilities/AccessModifierParser.hpp"
#include "utilities/AsyncValidator.hpp"
#include "utilities/VisibilityParser.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../ast/nodes/classes/ConstructorNode.hpp"
#include "../ast/nodes/classes/FieldNode.hpp"
#include "../ast/nodes/classes/SuperConstructorCallNode.hpp"
#include "../ast/nodes/classes/NewNode.hpp"
#include "../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../errors/ParseException.hpp"
#include "../errors/DuplicateDeclarationException.hpp"

namespace parser
{
    using namespace ast::nodes::classes;
    using namespace ast::nodes::expressions;
    using namespace token;
    using namespace value;
    using namespace errors;
    using namespace parser::utilities;

    ClassParser::ClassParser(TokenStream& stream, ParseContext& ctx)
        : tokenStream(stream), context(ctx),
          genericParameterParser(std::make_unique<GenericParameterParser>(stream, ctx))
    {
    }

    ClassParser::~ClassParser() = default;

    // ============================================================
    // Public entry: parseClass
    // ============================================================

    std::unique_ptr<ASTNode> ClassParser::parseClass()
    {
        auto annotations = AnnotationParser::parseAnnotations(tokenStream);

        validateClassDeclarationContext();

        std::unique_ptr<ASTNode> classNode;
        auto* classNodePtr = parseAndValidateClassHeader(classNode);
        const std::string& className = classNodePtr->getClassName();

        for (auto& annotation : annotations)
        {
            classNodePtr->addAnnotation(annotation);
        }

        std::unordered_set<std::string> declaredStaticMethodSignatures;
        std::unordered_set<std::string> declaredInstanceMethodSignatures;

        ParserContextState::ClassContextGuard classGuard(context.getContextState());

        parseClassMembers(classNodePtr, className,
                          declaredStaticMethodSignatures, declaredInstanceMethodSignatures);

        tokenStream.expect(TokenType::RBRACE);
        return classNode;
    }

    void ClassParser::validateClassDeclarationContext()
    {
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
        if (context.isInsideFunctionBody())
        {
            throw ParseException("Class declarations inside function bodies are not allowed. "
                                 "Declare classes at file scope.",
                                 tokenStream.current().location);
        }
    }

    ast::nodes::classes::ClassNode* ClassParser::parseAndValidateClassHeader(std::unique_ptr<ASTNode>& classNode)
    {
        classNode = parseClassDeclaration();
        auto* classNodePtr = dynamic_cast<ClassNode*>(classNode.get());

        if (!classNodePtr)
        {
            throw ParseException("Failed to create class node", tokenStream.current().location);
        }

        const std::string& className = classNodePtr->getClassName();
        const SourceLocation& classLocation = classNodePtr->getLocation();

        if (context.isTypeDeclared(className))
        {
            SourceLocation firstLocation = context.getTypeDeclarationLocation(className);
            throw DuplicateDeclarationException(
                "class",
                className,
                firstLocation,
                classLocation
            );
        }

        context.registerClass(className, classNodePtr->isFinal(), classLocation);

        return classNodePtr;
    }

    void ClassParser::parseClassMembers(
        ast::nodes::classes::ClassNode* classNodePtr,
        const std::string& className,
        std::unordered_set<std::string>& declaredStaticMethodSignatures,
        std::unordered_set<std::string>& declaredInstanceMethodSignatures)
    {
        while (tokenStream.current().type != TokenType::RBRACE && tokenStream.current().type != TokenType::END)
        {
            auto annotations = AnnotationParser::parseAnnotations(tokenStream);

            TokenType currentToken = tokenStream.current().type;

            if (currentToken == TokenType::CONSTRUCTOR ||
                ((currentToken == TokenType::PUBLIC || currentToken == TokenType::PRIVATE || currentToken ==
                        TokenType::PROTECTED) &&
                    tokenStream.peekAhead(1).type == TokenType::CONSTRUCTOR))
            {
                auto constructor = parseConstructor();
                if (constructor)
                {
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
            else if (isMethodDeclaration(currentToken))
            {
                auto method = parseMethod();
                if (method)
                {
                    auto* methodNode = dynamic_cast<ast::nodes::classes::MethodNode*>(method.get());
                    if (methodNode)
                    {
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
                auto field = parseField();
                if (field)
                {
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

        std::string signature = methodName + "(";
        const auto& params = methodNode->getGenericParameters();
        for (size_t i = 0; i < params.size(); ++i)
        {
            if (i > 0) signature += ",";
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
        if (currentToken == TokenType::FUNCTION)
        {
            return true;
        }

        if (currentToken == TokenType::STATIC)
        {
            TokenType next = tokenStream.peekAhead(1).type;
            if (next == TokenType::FUNCTION) return true;
            if (next == TokenType::FINAL && tokenStream.peekAhead(2).type == TokenType::FUNCTION) return true;
        }

        if (currentToken == TokenType::FINAL)
        {
            TokenType next = tokenStream.peekAhead(1).type;
            if (next == TokenType::FUNCTION) return true;
            if (next == TokenType::STATIC && tokenStream.peekAhead(2).type == TokenType::FUNCTION) return true;
        }

        if (currentToken == TokenType::ABSTRACT)
        {
            TokenType next = tokenStream.peekAhead(1).type;
            if (next == TokenType::FUNCTION) return true;
        }

        if (currentToken == TokenType::PUBLIC || currentToken == TokenType::PRIVATE || currentToken ==
            TokenType::PROTECTED)
        {
            TokenType next1 = tokenStream.peekAhead(1).type;

            if (next1 == TokenType::FUNCTION) return true;

            if (next1 == TokenType::STATIC)
            {
                TokenType next2 = tokenStream.peekAhead(2).type;
                if (next2 == TokenType::FUNCTION) return true;
                if (next2 == TokenType::FINAL && tokenStream.peekAhead(3).type == TokenType::FUNCTION) return true;
            }

            if (next1 == TokenType::FINAL)
            {
                TokenType next2 = tokenStream.peekAhead(2).type;
                if (next2 == TokenType::FUNCTION) return true;
                if (next2 == TokenType::STATIC && tokenStream.peekAhead(3).type == TokenType::FUNCTION) return true;
            }

            if (next1 == TokenType::ABSTRACT)
            {
                TokenType next2 = tokenStream.peekAhead(2).type;
                if (next2 == TokenType::FUNCTION) return true;
            }
        }

        return false;
    }

    // ============================================================
    // Class declaration (absorbed from ClassDeclarationParser)
    // ============================================================

    std::unique_ptr<ASTNode> ClassParser::parseClassDeclaration()
    {
        VisibilityModifier visibility = VisibilityParser::parseVisibilityModifier(tokenStream);

        bool isAbstract = false;
        if (tokenStream.check(TokenType::ABSTRACT))
        {
            isAbstract = true;
            tokenStream.advance();
        }

        bool isFinal = false;
        if (tokenStream.check(TokenType::FINAL))
        {
            isFinal = true;
            tokenStream.advance();
        }

        bool isValueClass = false;
        if (tokenStream.current().type == TokenType::IDENTIFIER &&
            tokenStream.current().stringValue == "value" &&
            tokenStream.peek().type == TokenType::CLASS)
        {
            isValueClass = true;
            tokenStream.advance();
        }

        if (isAbstract && isFinal)
        {
            throw ParseException(
                "Class cannot be both abstract and final. Abstract classes are meant to be extended, "
                "while final classes cannot be extended.",
                tokenStream.current().location);
        }

        if (isValueClass && isAbstract)
        {
            throw ParseException(
                "Class cannot be both value and abstract. Value classes have copy semantics "
                "and cannot participate in inheritance hierarchies.",
                tokenStream.current().location);
        }

        if (isValueClass)
        {
            isFinal = true;
        }

        tokenStream.expect(TokenType::CLASS);

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected class name", tokenStream.current().location);
        }

        std::string className = std::string(tokenStream.current().stringValue);
        SourceLocation classNameLocation = tokenStream.current().location;
        validateClassName(className, classNameLocation);
        tokenStream.advance();

        std::vector<ast::GenericTypeParameter> genericParameters;
        if (tokenStream.check(TokenType::LESS))
        {
            tokenStream.advance();
            genericParameters = genericParameterParser->parseGenericTypeParameters();
            tokenStream.expectGreaterForGeneric();

            if (genericParameters.size() > constants::MAX_GENERIC_PARAMETERS)
            {
                throw ParseException(
                    "Class '" + className + "' has too many generic parameters (" +
                    std::to_string(genericParameters.size()) + "). Maximum allowed: " +
                    std::to_string(constants::MAX_GENERIC_PARAMETERS),
                    tokenStream.current().location);
            }
        }

        std::string parentClassName = parseExtendsClause();

        if (isValueClass && !parentClassName.empty())
        {
            throw ParseException(
                "Value class '" + className + "' cannot extend another class. "
                "Value classes do not participate in inheritance hierarchies.",
                tokenStream.current().location);
        }

        if (!parentClassName.empty())
        {
            std::string baseParentName = parentClassName;
            size_t genericStart = parentClassName.find('<');
            if (genericStart != std::string::npos)
            {
                baseParentName = parentClassName.substr(0, genericStart);
            }

            if (context.isInterfaceDeclared(baseParentName))
            {
                throw ParseException(
                    "Class '" + className + "' cannot extend interface '" + baseParentName + "'. "
                    "Classes can only extend other classes. Use 'implements' for interfaces.",
                    tokenStream.current().location);
            }

            if (context.isClassFinal(baseParentName))
            {
                throw ParseException(
                    "Class '" + className + "' cannot extend final class '" + baseParentName + "'.",
                    tokenStream.current().location);
            }

            if (!context.registerClassInheritance(className, baseParentName))
            {
                auto chain = context.getClassInheritanceChain(className);
                std::string chainStr = className;
                for (const auto& ancestor : chain)
                {
                    if (ancestor != className)
                    {
                        chainStr += " -> " + ancestor;
                    }
                }
                chainStr += " -> " + baseParentName + " (creates cycle)";

                throw ParseException(
                    "Circular inheritance detected: " + chainStr,
                    tokenStream.current().location);
            }
        }

        std::vector<std::string> implementedInterfaces = parseImplementedInterfaces();

        tokenStream.expect(TokenType::LBRACE);

        auto classNode = std::make_unique<ClassNode>(className, genericParameters, parentClassName,
                                                     implementedInterfaces, classNameLocation);
        classNode->setFinal(isFinal);
        classNode->setAbstract(isAbstract);
        classNode->setValueClass(isValueClass);
        classNode->setVisibility(visibility);
        return classNode;
    }

    std::string ClassParser::parseExtendsClause()
    {
        if (!tokenStream.check(TokenType::EXTENDS))
        {
            return "";
        }

        tokenStream.advance();

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected parent class name after 'extends'",
                                 tokenStream.current().location);
        }

        std::string parentClass = parseGenericInterfaceName();

        NameValidator::validateCapitalizedName(parentClass.substr(0, parentClass.find('<')),
                                             "Parent class",
                                             tokenStream.current().location);

        return parentClass;
    }

    std::vector<std::string> ClassParser::parseImplementedInterfaces()
    {
        std::vector<std::string> implementedInterfaces;

        if (tokenStream.check(TokenType::IMPLEMENTS))
        {
            tokenStream.advance();
            implementedInterfaces = ParserUtils::parseInterfaceList(tokenStream, "implements");
        }

        return implementedInterfaces;
    }

    std::string ClassParser::parseQualifiedClassName()
    {
        std::vector<std::string> qualifiedParts;
        qualifiedParts.push_back(std::string(tokenStream.current().stringValue));
        tokenStream.advance();

        while (tokenStream.current().type == TokenType::SCOPE)
        {
            tokenStream.advance();
            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected identifier after '::'", tokenStream.current().location);
            }
            qualifiedParts.push_back(std::string(tokenStream.current().stringValue));
            tokenStream.advance();
        }

        std::string className = qualifiedParts[0];
        for (size_t i = 1; i < qualifiedParts.size(); ++i)
        {
            className += "::" + qualifiedParts[i];
        }

        return className;
    }

    void ClassParser::validateClassName(const std::string& className, const SourceLocation& location)
    {
        NameValidator::validateCapitalizedName(className, "Class", location);
    }

    std::string ClassParser::parseGenericInterfaceName()
    {
        std::string interfaceName = std::string(tokenStream.current().stringValue);
        tokenStream.advance();

        if (tokenStream.check(TokenType::LESS))
        {
            interfaceName += ParserUtils::parseNestedGenericExpression(tokenStream);
        }

        return interfaceName;
    }

    // ============================================================
    // Constructor (absorbed from ConstructorParser)
    // ============================================================

    std::unique_ptr<ASTNode> ClassParser::parseConstructor()
    {
        ast::AccessModifier accessModifier =
            AccessModifierParser::parseAccessModifier(tokenStream, ast::AccessModifier::PUBLIC);

        auto constructorLocation = tokenStream.current().location;
        tokenStream.expect(TokenType::CONSTRUCTOR);

        auto paramDecls = ParameterParser::parseTypedParameterDeclList(tokenStream, true);
        std::vector<std::pair<std::string, value::ParameterType>> parametersWithTypes;
        std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> parameterAnnotations;
        parametersWithTypes.reserve(paramDecls.size());
        parameterAnnotations.reserve(paramDecls.size());
        for (auto& d : paramDecls) {
            parametersWithTypes.emplace_back(d.name, d.type);
            parameterAnnotations.push_back(std::move(d.annotations));
        }

        std::unique_ptr<SuperConstructorCallNode> superInitializer = nullptr;
        if (tokenStream.check(TokenType::COLON))
        {
            tokenStream.advance();

            if (!tokenStream.check(TokenType::SUPER))
            {
                throw ParseException(
                    "Expected 'super' after ':' in constructor initializer list",
                    tokenStream.current().location);
            }

            auto superLocation = tokenStream.current().location;
            tokenStream.advance();

            if (!tokenStream.check(TokenType::LPAREN))
            {
                throw ParseException(
                    "Expected '(' after 'super' in constructor initializer",
                    tokenStream.current().location);
            }

            tokenStream.advance();

            std::vector<std::unique_ptr<ASTNode>> arguments;

            if (!tokenStream.check(TokenType::RPAREN))
            {
                arguments.push_back(context.parseExpression());
                while (tokenStream.check(TokenType::COMMA))
                {
                    tokenStream.advance();
                    arguments.push_back(context.parseExpression());
                }
            }

            tokenStream.expect(TokenType::RPAREN);

            superInitializer = std::make_unique<SuperConstructorCallNode>(
                std::move(arguments), superLocation);
        }

        ParseContext::ConstructorContextGuard constructorGuard(context.getContextState());
        auto body = context.parseStatement();

        auto constructorNode = std::make_unique<ConstructorNode>(
            std::move(parametersWithTypes), std::move(body),
            accessModifier, constructorLocation);

        if (superInitializer)
        {
            constructorNode->setSuperInitializer(std::move(superInitializer));
        }

        constructorNode->setParameterAnnotations(std::move(parameterAnnotations));

        return constructorNode;
    }

    // ============================================================
    // Method (absorbed from MethodParser)
    // ============================================================

    std::unique_ptr<ASTNode> ClassParser::parseMethod()
    {
        SourceLocation methodStartLocation = tokenStream.current().location;

        ast::AccessModifier accessModifier =
            AccessModifierParser::parseAccessModifier(tokenStream, ast::AccessModifier::PRIVATE);

        bool isFinal = false;
        if (tokenStream.current().type == TokenType::FINAL)
        {
            isFinal = true;
            tokenStream.advance();
        }

        bool isAbstract = false;
        if (tokenStream.current().type == TokenType::ABSTRACT)
        {
            isAbstract = true;
            tokenStream.advance();
        }

        bool isStatic = false;
        if (tokenStream.current().type == TokenType::STATIC)
        {
            isStatic = true;
            tokenStream.advance();
        }

        if (isAbstract && isFinal)
        {
            throw ParseException(
                "Method cannot be both abstract and final. "
                "Abstract methods must be overridden, while final methods cannot be overridden.",
                tokenStream.current().location);
        }

        if (isAbstract && isStatic)
        {
            throw ParseException(
                "Method cannot be both abstract and static. Only instance methods can be abstract.",
                tokenStream.current().location);
        }

        return parseMethodWithModifiers(accessModifier, isStatic, false, isAbstract, isFinal, methodStartLocation);
    }

    std::unique_ptr<ASTNode> ClassParser::parseMethodWithModifiers(ast::AccessModifier accessModifier, bool isStatic,
                                                                    bool isAsync, bool isAbstract, bool isFinal,
                                                                    const SourceLocation& methodStartLocation)
    {
        if (tokenStream.current().type != TokenType::FUNCTION)
        {
            throw ParseException("Expected 'function' keyword", tokenStream.current().location);
        }
        tokenStream.advance();

        if (tokenStream.current().type == TokenType::ASYNC)
        {
            isAsync = true;
            tokenStream.advance();
        }

        std::vector<GenericTypeParameter> methodGenericParameters = parseMethodGenericParameters();

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected method name", tokenStream.current().location);
        }

        std::string methodName = std::string(tokenStream.current().stringValue);
        validateMethodName(methodName, isStatic);
        tokenStream.advance();

        auto paramDecls = ParameterParser::parseGenericParameterDeclList(tokenStream, true);
        std::vector<std::pair<std::string, std::shared_ptr<GenericType>>> parameters;
        std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> parameterAnnotations;
        parameters.reserve(paramDecls.size());
        parameterAnnotations.reserve(paramDecls.size());
        for (auto& d : paramDecls) {
            parameters.emplace_back(d.name, d.type);
            parameterAnnotations.push_back(std::move(d.annotations));
        }

        std::shared_ptr<GenericType> returnType = std::make_shared<GenericType>(ValueType::VOID);
        if (tokenStream.current().type == TokenType::COLON)
        {
            tokenStream.advance();
            returnType = TypeParser::parseGenericType(tokenStream);
        }

        if (isAsync)
        {
            AsyncValidator::validateAsyncReturnType(returnType, tokenStream.location());
        }

        std::unique_ptr<ASTNode> body = nullptr;

        if (isAbstract)
        {
            if (tokenStream.current().type != TokenType::SEMICOLON)
            {
                throw ParseException(
                    "Abstract method '" + methodName + "' must not have a body. "
                    "Expected semicolon after method signature.",
                    tokenStream.current().location);
            }
            tokenStream.advance();
        }
        else
        {
            ParseContext::AsyncContextGuard asyncGuard(context.getContextState(), isAsync);
            body = context.parseStatement();
        }

        auto methodNode = std::make_unique<MethodNode>(methodName, returnType, std::move(parameters),
                                                       std::move(body), isStatic, methodGenericParameters,
                                                       accessModifier, isAsync, methodStartLocation);
        methodNode->setAbstract(isAbstract);
        methodNode->setFinal(isFinal);
        methodNode->setParameterAnnotations(std::move(parameterAnnotations));
        return methodNode;
    }

    std::vector<GenericTypeParameter> ClassParser::parseMethodGenericParameters()
    {
        std::vector<GenericTypeParameter> methodGenericParameters;
        if (tokenStream.check(TokenType::LESS))
        {
            tokenStream.advance();
            methodGenericParameters = genericParameterParser->parseGenericTypeParameters();
            tokenStream.expectGreaterForGeneric();
        }
        return methodGenericParameters;
    }

    void ClassParser::validateMethodName(const std::string& methodName, bool isStatic)
    {
        std::string methodType = isStatic ? "Static method" : "Method";
        NameValidator::validateFunctionNamingConvention(methodName, isStatic, methodType, tokenStream.location());
    }

    // ============================================================
    // Field (absorbed from FieldParser)
    // ============================================================

    std::unique_ptr<ASTNode> ClassParser::parseField()
    {
        auto [accessModifier, isStatic, isFinal] = parseFieldModifiers();

        if (tokenStream.current().type == TokenType::FUNCTION)
        {
            throw ParseException("Unexpected 'function' keyword in field declaration context. "
                                 "This should have been handled by MethodParser.",
                                 tokenStream.current().location);
        }

        return parseFieldDeclaration(accessModifier, isStatic, isFinal);
    }

    std::tuple<ast::AccessModifier, bool, bool> ClassParser::parseFieldModifiers()
    {
        ast::AccessModifier accessModifier =
            AccessModifierParser::parseAccessModifier(tokenStream, ast::AccessModifier::PRIVATE);

        bool isStatic = false;
        bool isFinal = false;

        if (tokenStream.current().type == TokenType::STATIC)
        {
            isStatic = true;
            tokenStream.advance();
        }

        if (tokenStream.current().type == TokenType::FINAL)
        {
            isFinal = true;
            tokenStream.advance();
        }

        return {accessModifier, isStatic, isFinal};
    }

    std::unique_ptr<ASTNode> ClassParser::parseFieldDeclaration(ast::AccessModifier accessModifier, bool isStatic,
                                                                bool isFinal)
    {
        auto fieldGenericType = TypeParser::parseGenericType(tokenStream);

        if (tokenStream.current().type != TokenType::IDENTIFIER)
        {
            throw ParseException("Expected field name", tokenStream.current().location);
        }

        std::string fieldName = std::string(tokenStream.current().stringValue);
        SourceLocation fieldLocation = tokenStream.current().location;

        NameValidator::validateIdentifierName(fieldName, "Field", fieldLocation);

        tokenStream.advance();

        std::unique_ptr<ASTNode> initialValue = parseInitialValue();

        tokenStream.expect(TokenType::SEMICOLON);

        return std::make_unique<FieldNode>(fieldName, fieldGenericType, std::move(initialValue),
                                           isStatic, isFinal, accessModifier, fieldLocation);
    }

    std::unique_ptr<ASTNode> ClassParser::parseInitialValue()
    {
        std::unique_ptr<ASTNode> initialValue = nullptr;
        if (tokenStream.current().type == TokenType::ASSIGN)
        {
            tokenStream.advance();
            initialValue = context.parseExpression();
        }
        return initialValue;
    }

    // ============================================================
    // Object creation (absorbed from ObjectCreationParser)
    // ============================================================

    std::unique_ptr<ASTNode> ClassParser::parseNewExpression()
    {
        auto newLocation = tokenStream.current().location;
        tokenStream.expect(TokenType::NEW);

        TokenType currentType = tokenStream.current().type;
        if (currentType != TokenType::IDENTIFIER &&
            currentType != TokenType::INT &&
            currentType != TokenType::FLOAT &&
            currentType != TokenType::BOOL &&
            currentType != TokenType::STRING_TYPE)
        {
            throw ParseException("Expected class name after 'new'", tokenStream.current().location);
        }

        std::string className = parseClassNameWithGenerics();

        if (tokenStream.check(TokenType::LBRACKET))
        {
            return parseArrayCreation(className);
        }

        return parseObjectInstantiation(className);
    }

    std::unique_ptr<ASTNode> ClassParser::parseArrayCreation(const std::string& className)
    {
        std::vector<std::unique_ptr<ASTNode>> sizeExpressions = parseArrayDimensions();
        TypeInfo elementTypeInfo = createElementTypeInfo(className);
        auto newLocation = tokenStream.current().location;

        return std::make_unique<ArrayCreationNode>(elementTypeInfo, std::move(sizeExpressions), newLocation);
    }

    std::unique_ptr<ASTNode> ClassParser::parseObjectInstantiation(const std::string& className)
    {
        std::vector<std::string> qualifiedParts;
        size_t scopePos = className.find("::");
        if (scopePos != std::string::npos)
        {
            qualifiedParts.push_back(className.substr(scopePos + 2));
        }
        else
        {
            qualifiedParts.push_back(className);
        }

        std::string finalClassName = qualifiedParts.back();
        validateClassNameForInstantiation(finalClassName);

        std::vector<std::unique_ptr<ASTNode>> arguments = parseConstructorArguments();
        auto newLocation = tokenStream.current().location;

        return std::make_unique<NewNode>(className, std::move(arguments), newLocation);
    }

    std::string ClassParser::parseClassNameWithGenerics()
    {
        std::vector<std::string> qualifiedParts;
        qualifiedParts.push_back(std::string(tokenStream.current().stringValue));
        tokenStream.advance();

        while (tokenStream.current().type == TokenType::SCOPE)
        {
            tokenStream.advance();
            if (tokenStream.current().type != TokenType::IDENTIFIER)
            {
                throw ParseException("Expected identifier after '::'", tokenStream.current().location);
            }
            qualifiedParts.push_back(std::string(tokenStream.current().stringValue));
            tokenStream.advance();
        }

        std::string className = qualifiedParts[0];
        for (size_t i = 1; i < qualifiedParts.size(); ++i)
        {
            className += "::" + qualifiedParts[i];
        }

        if (tokenStream.check(TokenType::LESS))
        {
            std::string genericsString = ParserUtils::parseNestedGenericExpression(tokenStream);
            className += genericsString;
        }

        return className;
    }

    std::vector<std::unique_ptr<ASTNode>> ClassParser::parseConstructorArguments()
    {
        tokenStream.expect(TokenType::LPAREN);
        std::vector<std::unique_ptr<ASTNode>> arguments;
        if (!tokenStream.check(TokenType::RPAREN))
        {
            arguments.push_back(context.parseExpression());
            while (tokenStream.check(TokenType::COMMA))
            {
                tokenStream.advance();
                arguments.push_back(context.parseExpression());
            }
        }
        tokenStream.expect(TokenType::RPAREN);
        return arguments;
    }

    std::vector<std::unique_ptr<ASTNode>> ClassParser::parseArrayDimensions()
    {
        std::vector<std::unique_ptr<ASTNode>> sizeExpressions;

        while (tokenStream.check(TokenType::LBRACKET))
        {
            tokenStream.advance();

            if (tokenStream.check(TokenType::RBRACKET))
            {
                sizeExpressions.push_back(nullptr);
            }
            else
            {
                auto sizeExpression = context.parseExpression();
                sizeExpressions.push_back(std::move(sizeExpression));
            }

            tokenStream.expect(TokenType::RBRACKET);
        }

        return sizeExpressions;
    }

    TypeInfo ClassParser::createElementTypeInfo(const std::string& className)
    {
        if (className == "int") return TypeInfo(ValueType::INT);
        if (className == "float") return TypeInfo(ValueType::FLOAT);
        if (className == "bool") return TypeInfo(ValueType::BOOL);
        if (className == "string") return TypeInfo(ValueType::STRING);

        return TypeInfo(ValueType::OBJECT, className);
    }

    void ClassParser::validateClassNameForInstantiation(const std::string& finalClassName)
    {
        if (!ParserValidator::isValidClassName(finalClassName))
        {
            throw ParseException("Class name '" + finalClassName + "' must start with an uppercase letter",
                                 tokenStream.current().location);
        }
    }
}
