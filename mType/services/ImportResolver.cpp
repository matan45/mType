#include "ImportResolver.hpp"
#include "ImportManager.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../environment/registry/ClassDefinition.hpp"
#include "../environment/registry/FunctionDefinition.hpp"
#include <stdexcept>

namespace services
{
    ImportResolver::ImportResolver(std::shared_ptr<environment::Environment> env)
        : environment(env)
    {
    }

    ImportResolver::~ImportResolver() = default;

    void ImportResolver::resolveImports(ast::ASTNode* ast)
    {
        // Recursively resolve all imports in the AST
        // This ensures that all ImportNode objects have their importedAST set
        // before bytecode compilation

        if (!ast) return;

        auto importManager = environment->getImportManager();
        if (!importManager)
        {
            throw std::runtime_error("ImportManager not available for import resolution");
        }

        // Check if this is an ImportNode
        if (auto importNode = dynamic_cast<ast::ImportNode*>(ast))
        {
            std::string filePath = importNode->getFilePath();

            // Skip if already resolved
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                // Recursively resolve imports in the imported AST
                resolveImports(importNode->getImportedAST());
                return;
            }

            std::string resolvedPath = importManager->resolvePath(filePath);

            // Check for circular imports
            if (importManager->isBeingEvaluated(resolvedPath))
            {
                throw std::runtime_error("Circular import detected: " + filePath);
            }

            // Mark as being evaluated
            importManager->markAsBeingEvaluated(resolvedPath);

            // Save the current file path and set it to the file being imported
            // This ensures that nested imports in the imported file are resolved correctly
            std::string savedCurrentFile = importManager->getCurrentFilePath();

            try
            {
                // Parse and cache the imported AST BEFORE setting current file
                // This ensures parseAndCacheAST uses the correct context
                ast::ASTNode* importedAST = importManager->parseAndCacheAST(filePath);

                if (!importedAST)
                {
                    throw std::runtime_error("Failed to parse import: " + filePath);
                }

                // Set current file to the resolved path AFTER parsing
                // This is critical for nested imports to resolve correctly
                importManager->setCurrentFilePath(resolvedPath);

                // Set the imported AST on the ImportNode
                importNode->setImportedAST(importedAST);

                // Recursively resolve imports in the imported file
                resolveImports(importedAST);

                // Pre-register class definitions from the imported AST
                // This is needed for bytecode compilation to find classes
                preRegisterClassDefinitions(importedAST);

                // Restore the previous current file
                importManager->setCurrentFilePath(savedCurrentFile);

                // Mark as evaluated
                importManager->markAsEvaluated(resolvedPath);
            }
            catch (...)
            {
                // Restore the previous current file on error
                importManager->setCurrentFilePath(savedCurrentFile);
                // Unmark as being evaluated on error
                importManager->unmarkAsBeingEvaluated(resolvedPath);
                throw;
            }

            // Unmark as being evaluated (successful completion)
            importManager->unmarkAsBeingEvaluated(resolvedPath);
            return;
        }

        // Recursively process child nodes
        // Check for ProgramNode
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(ast))
        {
            const auto& statements = programNode->getStatements();
            for (const auto& stmt : statements)
            {
                resolveImports(stmt.get());
            }
            return;
        }

        // Check for BlockNode
        if (auto blockNode = dynamic_cast<ast::BlockNode*>(ast))
        {
            const auto& statements = blockNode->getStatements();
            for (const auto& stmt : statements)
            {
                resolveImports(stmt.get());
            }
            return;
        }

        // For other node types, we don't need to traverse further
        // Imports are typically at the top level
    }

    void ImportResolver::preRegisterClassDefinitions(ast::ASTNode* node)
    {
        if (!node) return;

        // Check if this node is a ClassNode
        if (auto classNode = dynamic_cast<ast::ClassNode*>(node))
        {
            std::string className = classNode->getClassName();

            // Note: In bytecode mode, classes are registered during compilation by BytecodeCompiler's ClassRegistrar
            // This pre-registration is no longer needed since we're bytecode-only

            // Register static methods as global functions
            registerStaticMethodsAsGlobalFunctions(className, classNode);

            return; // No need to traverse children of ClassNode
        }

        // Recursively process child nodes
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                preRegisterClassDefinitions(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                preRegisterClassDefinitions(statement.get());
            }
        }
        // Add other node types that may contain ClassNodes as needed
    }

    void ImportResolver::registerStaticMethodsAsGlobalFunctions(const std::string& className,
                                                                ast::ClassNode* classNode)
    {
        auto classDef = environment->findClass(className);
        if (!classDef) return;

        auto funcRegistry = environment->getFunctionRegistry();
        if (!funcRegistry) return;

        const auto& staticMethods = classDef->getStaticMethods();

        for (const auto& [methodName, overloads] : staticMethods)
        {
            std::string qualifiedName = className + "::" + methodName;

            for (const auto& method : overloads)
            {
                if (!method) continue;

                auto funcDef = std::make_shared<runtimeTypes::global::FunctionDefinition>(
                    qualifiedName,
                    method->getReturnType(),
                    method->getParametersWithTypes()
                );

                funcDef->setBody(method->getBodyPtr());
                funcRegistry->registerFunction(qualifiedName, funcDef);
            }
        }
    }
}
