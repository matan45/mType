#pragma once
#include "../ast/ASTNode.hpp"
#include "TypeRegistry.hpp"
#include "ParserContextState.hpp"
#include <memory>
#include <string>
#include <vector>

namespace parser
{
    class Parser;

    using namespace ast;

    class ParseContext
    {
    private:
        Parser* parser = nullptr;

        TypeRegistry typeRegistry;
        ParserContextState contextState;

    public:
        explicit ParseContext(Parser* p) : parser(p) {}

        ~ParseContext() = default;

        [[nodiscard]] std::unique_ptr<ASTNode> parseStatement();
        [[nodiscard]] std::unique_ptr<ASTNode> parseExpression();
        [[nodiscard]] std::unique_ptr<ASTNode> parseClass();
        [[nodiscard]] std::unique_ptr<ASTNode> parseInterface();
        [[nodiscard]] std::unique_ptr<ASTNode> parseAnnotationDeclaration();
        [[nodiscard]] std::unique_ptr<ASTNode> parseNewExpression();

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
