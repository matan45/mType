#pragma once
#include <memory>
#include "../ast/ASTNode.hpp"
#include "../environment/Environment.hpp"

namespace services
{
    class ImportManager;

    /**
     * Service for resolving imports and pre-registering class definitions
     * Handles the import resolution phase before evaluation/compilation
     */
    class ImportResolver
    {
    private:
        std::shared_ptr<environment::Environment> environment;

        // Pre-register class definitions from an AST node
        void preRegisterClassDefinitions(ast::ASTNode* node);

        // Register static methods as global functions for bytecode mode
        void registerStaticMethodsAsGlobalFunctions(const std::string& className,
                                                    ast::ClassNode* classNode);

    public:
        explicit ImportResolver(std::shared_ptr<environment::Environment> env);
        ~ImportResolver();

        // Resolve all imports in an AST recursively
        void resolveImports(ast::ASTNode* ast);
    };
}
