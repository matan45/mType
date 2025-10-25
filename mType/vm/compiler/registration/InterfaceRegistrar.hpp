#pragma once
#include "../../../ast/ASTNode.hpp"
#include "../../../ast/nodes/classes/InterfaceNode.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../../../environment/Environment.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../types/GenericTypeResolver.hpp"
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

namespace vm::compiler::registration
{
    /**
     * Registers interfaces and validates interface implementations
     * Handles interface definition creation and method signature validation
     */
    class InterfaceRegistrar
    {
    public:
        InterfaceRegistrar(
            std::shared_ptr<environment::Environment> environment,
            const types::GenericTypeResolver& genericResolver
        );

        ~InterfaceRegistrar() = default;

        // Interface registration
        void registerInterfaceForBytecode(ast::nodes::classes::InterfaceNode* interfaceNode);

        // Traverse AST and register all interfaces
        void registerInterfaces(ast::ASTNode* node);

        // Interface implementation validation
        void validateInterfaceImplementations(
            std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
            ast::ClassNode* classNode
        );

    private:
        std::shared_ptr<environment::Environment> environment;
        const types::GenericTypeResolver& genericResolver;

        // Helper methods
        std::pair<std::string, std::vector<std::string>> parseGenericInterfaceName(
            const std::string& interfaceName
        ) const;

        std::string resolveGenericType(
            const std::string& typeName,
            const std::unordered_map<std::string, std::string>& substitutions
        ) const;
    };
}
