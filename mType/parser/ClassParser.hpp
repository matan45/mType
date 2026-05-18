#pragma once
#include <memory>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>
#include "../ast/ASTNode.hpp"
#include "../ast/AccessModifier.hpp"
#include "../ast/GenericTypeParameter.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../errors/SourceLocation.hpp"
#include "../token/TokenType.hpp"
#include "../value/ParameterType.hpp"
#include "TokenStream.hpp"
#include "ParseContext.hpp"

namespace parser
{
    class GenericParameterParser;
    struct TypeInfo;
    using namespace ast;

    class ClassParser
    {
    private:
        TokenStream& tokenStream;
        ParseContext& context;

        std::unique_ptr<GenericParameterParser> genericParameterParser;

    public:
        explicit ClassParser(TokenStream& stream, ParseContext& ctx);
        ~ClassParser();

        std::unique_ptr<ASTNode> parseClass();
        std::unique_ptr<ASTNode> parseConstructor();
        std::unique_ptr<ASTNode> parseMethod();
        std::unique_ptr<ASTNode> parseField();
        std::unique_ptr<ASTNode> parseNewExpression();

    private:
        // parseClass orchestration
        void validateClassDeclarationContext();
        ast::nodes::classes::ClassNode* parseAndValidateClassHeader(std::unique_ptr<ASTNode>& classNode);
        void parseClassMembers(
            ast::nodes::classes::ClassNode* classNodePtr,
            const std::string& className,
            std::unordered_set<std::string>& declaredStaticMethodSignatures,
            std::unordered_set<std::string>& declaredInstanceMethodSignatures);
        void validateAndRegisterMethodSignature(
            ast::nodes::classes::MethodNode* methodNode,
            const std::string& className,
            std::unordered_set<std::string>& declaredStaticMethodSignatures,
            std::unordered_set<std::string>& declaredInstanceMethodSignatures);
        [[nodiscard]] std::string buildMethodSignature(const ast::nodes::classes::MethodNode* methodNode) const;
        bool isMethodDeclaration(token::TokenType currentToken) const;

        // Class declaration parsing (from ClassDeclarationParser)
        std::unique_ptr<ASTNode> parseClassDeclaration();
        std::vector<std::string> parseImplementedInterfaces();
        std::string parseQualifiedClassName();
        void validateClassName(const std::string& className, const errors::SourceLocation& location);
        std::string parseGenericInterfaceName();
        std::string parseExtendsClause();

        // Method parsing (from MethodParser)
        std::unique_ptr<ASTNode> parseMethodWithModifiers(ast::AccessModifier accessModifier, bool isStatic,
                                                          bool isAsync, bool isAbstract, bool isFinal,
                                                          const errors::SourceLocation& methodStartLocation);
        std::vector<GenericTypeParameter> parseMethodGenericParameters();
        void validateMethodName(const std::string& methodName, bool isStatic);

        // Field parsing (from FieldParser)
        std::tuple<ast::AccessModifier, bool, bool> parseFieldModifiers();
        std::unique_ptr<ASTNode> parseFieldDeclaration(ast::AccessModifier accessModifier, bool isStatic, bool isFinal);
        std::unique_ptr<ASTNode> parseInitialValue();

        // Object creation (from ObjectCreationParser)
        std::unique_ptr<ASTNode> parseArrayCreation(const std::string& className);
        std::unique_ptr<ASTNode> parseObjectInstantiation(const std::string& className);
        std::string parseClassNameWithGenerics();
        std::vector<std::unique_ptr<ASTNode>> parseConstructorArguments();
        std::vector<std::unique_ptr<ASTNode>> parseArrayDimensions();
        TypeInfo createElementTypeInfo(const std::string& className);
        void validateClassNameForInstantiation(const std::string& finalClassName);
    };
}
