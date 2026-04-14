#pragma once
#include "../ast/ASTNode.hpp"
#include "TypeRegistry.hpp"
#include "ParserContextState.hpp"
#include <memory>
#include <optional>
#include <functional>
#include <string>
#include <vector>

namespace parser
{
    class StatementParser;
    class ExpressionParser;
    class ClassParser;
    class InterfaceParser;
    class AnnotationDeclarationParser;
    class TokenStream;

    using namespace ast;

    /// @brief Facade for inter-parser communication without circular dependencies
    /// Coordinates between parsers using TypeRegistry and ParserContextState
    class ParseContext
    {
    private:
        std::optional<std::reference_wrapper<StatementParser>> statementParser;
        std::optional<std::reference_wrapper<ExpressionParser>> expressionParser;
        std::optional<std::reference_wrapper<ClassParser>> classParser;
        std::optional<std::reference_wrapper<InterfaceParser>> interfaceParser;
        std::optional<std::reference_wrapper<AnnotationDeclarationParser>> annotationDeclarationParser;
        std::optional<std::reference_wrapper<TokenStream>> tokenStream;

        // Composed components following SRP
        TypeRegistry typeRegistry;
        ParserContextState contextState;

    public:
        /// @brief Default constructor for delayed initialization
        explicit ParseContext() = default;

        /// @brief Constructor with immediate initialization
        explicit ParseContext(StatementParser& stmt, ExpressionParser& expr, ClassParser& cls, InterfaceParser& iface,
                              TokenStream& stream);

        ~ParseContext() = default;

        /// @brief Parse a statement using StatementParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseStatement();

        /// @brief Parse an expression using ExpressionParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseExpression();

        /// @brief Parse a class using ClassParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseClass();

        /// @brief Parse an interface using InterfaceParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseInterface();

        /// @brief Parse an annotation type declaration using AnnotationDeclarationParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseAnnotationDeclaration();

        /// @brief Parse a new expression using ClassParser
        [[nodiscard]] std::unique_ptr<ASTNode> parseNewExpression();

        // Setters for delayed initialization with memory-safe references
        void setStatementParser(StatementParser& parser);
        void setExpressionParser(ExpressionParser& parser);
        void setClassParser(ClassParser& parser);
        void setInterfaceParser(InterfaceParser& parser);
        void setAnnotationDeclarationParser(AnnotationDeclarationParser& parser);
        void setTokenStream(TokenStream& stream);

        // Context state delegation (delegates to ParserContextState)
        [[nodiscard]] bool isInsideAsyncFunction() const { return contextState.isInsideAsyncFunction(); }
        void setAsyncContext(bool async) { contextState.setAsyncContext(async); }

        [[nodiscard]] bool isInsideFunctionBody() const { return contextState.isInsideFunctionBody(); }
        void setFunctionContext(bool inFunction) { contextState.setFunctionContext(inFunction); }

        [[nodiscard]] bool isInsideClassBody() const { return contextState.isInsideClassBody(); }
        void setClassContext(bool inClass) { contextState.setClassContext(inClass); }

        [[nodiscard]] bool isInsideInterfaceBody() const { return contextState.isInsideInterfaceBody(); }
        void setInterfaceContext(bool inInterface) { contextState.setInterfaceContext(inInterface); }

        [[nodiscard]] bool isInsideConstructorBody() const { return contextState.isInsideConstructorBody(); }
        void setConstructorContext(bool inConstructor) { contextState.setConstructorContext(inConstructor); }

        // RAII Guards (delegates to ParserContextState)
        using AsyncContextGuard = ParserContextState::AsyncContextGuard;
        using FunctionContextGuard = ParserContextState::FunctionContextGuard;
        using ClassContextGuard = ParserContextState::ClassContextGuard;
        using InterfaceContextGuard = ParserContextState::InterfaceContextGuard;
        using ConstructorContextGuard = ParserContextState::ConstructorContextGuard;

        // Get state reference for RAII guards
        [[nodiscard]] ParserContextState& getContextState() { return contextState; }

        // Type registry delegation (delegates to TypeRegistry)
        [[nodiscard]] bool isTypeDeclared(const std::string& typeName) const
        {
            return typeRegistry.isTypeDeclared(typeName);
        }

        void registerTypeName(const std::string& typeName)
        {
            typeRegistry.registerTypeName(typeName);
        }

        void clearDeclaredTypes()
        {
            typeRegistry.clearDeclaredTypes();
        }

        [[nodiscard]] bool isClassDeclared(const std::string& className) const
        {
            return typeRegistry.isClassDeclared(className);
        }

        [[nodiscard]] bool isInterfaceDeclared(const std::string& interfaceName) const
        {
            return typeRegistry.isInterfaceDeclared(interfaceName);
        }

        void registerClass(const std::string& className, bool isFinal = false,
                          const errors::SourceLocation& location = errors::SourceLocation())
        {
            typeRegistry.registerClass(className, isFinal, location);
        }

        void registerInterface(const std::string& interfaceName, bool isFinal = false,
                              const errors::SourceLocation& location = errors::SourceLocation())
        {
            typeRegistry.registerInterface(interfaceName, isFinal, location);
        }

        [[nodiscard]] bool isClassFinal(const std::string& className) const
        {
            return typeRegistry.isClassFinal(className);
        }

        [[nodiscard]] bool isInterfaceFinal(const std::string& interfaceName) const
        {
            return typeRegistry.isInterfaceFinal(interfaceName);
        }

        bool registerClassInheritance(const std::string& childClass, const std::string& parentClass)
        {
            return typeRegistry.registerClassInheritance(childClass, parentClass);
        }

        bool registerInterfaceInheritance(const std::string& childInterface,
                                          const std::vector<std::string>& parentInterfaces)
        {
            return typeRegistry.registerInterfaceInheritance(childInterface, parentInterfaces);
        }

        [[nodiscard]] std::vector<std::string> getClassInheritanceChain(const std::string& className) const
        {
            return typeRegistry.getClassInheritanceChain(className);
        }

        [[nodiscard]] bool isFunctionDeclared(const std::string& functionName) const
        {
            return typeRegistry.isFunctionDeclared(functionName);
        }

        void registerFunctionName(const std::string& functionName,
                                 const errors::SourceLocation& location = errors::SourceLocation())
        {
            typeRegistry.registerFunctionName(functionName, location);
        }

        [[nodiscard]] errors::SourceLocation getTypeDeclarationLocation(const std::string& typeName) const
        {
            return typeRegistry.getTypeDeclarationLocation(typeName);
        }

        [[nodiscard]] errors::SourceLocation getFunctionDeclarationLocation(const std::string& functionName) const
        {
            return typeRegistry.getFunctionDeclarationLocation(functionName);
        }

        void clearDeclaredFunctions()
        {
            typeRegistry.clearDeclaredFunctions();
        }
    };
}
