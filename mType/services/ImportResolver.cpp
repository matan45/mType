#include "ImportResolver.hpp"
#include "ImportManager.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/classes/MethodNode.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/global/FunctionDefinition.hpp"
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
        // This allows static methods to be called via CALL opcode with qualified names
        for (const auto& method : classNode->getMethods())
        {
            if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get()))
            {
                if (methodNode->getIsStatic())
                {
                    // Get the class definition from environment
                    auto classDef = environment->findClass(className);
                    if (classDef)
                    {
                        // Check if this static method exists in the class
                        const auto& staticMethods = classDef->getStaticMethods();
                        auto it = staticMethods.find(methodNode->getName());
                        if (it != staticMethods.end() && !it->second.empty())
                        {
                            // Register as a global function with qualified name
                            // Note: For overloaded methods, we register the first one found (TODO: improve this)
                            const auto& method = it->second[0];
                            std::string qualifiedName = className + "::" + methodNode->getName();

                            auto funcRegistry = environment->getFunctionRegistry();
                            if (funcRegistry)
                            {
                                // Convert generic parameters to ParameterType format
                                std::vector<std::pair<std::string, value::ParameterType>> parameterTypes;
                                const auto& genericParams = methodNode->getGenericParameters();

                                for (const auto& [paramName, genericType] : genericParams)
                                {
                                    if (genericType)
                                    {
                                        // Extract ValueType from GenericType
                                        value::ValueType baseType = value::ValueType::VOID;
                                        if (genericType->isGenericParameter())
                                        {
                                            // For generic parameters (T, E, etc.), use OBJECT as the base type
                                            baseType = value::ValueType::OBJECT;
                                        }
                                        else
                                        {
                                            baseType = genericType->getConcreteType();
                                        }

                                        std::string typeName = genericType->getBaseTypeName();

                                        // Check if it's an interface or class type
                                        if (baseType == value::ValueType::OBJECT)
                                        {
                                            if (environment->findInterface(typeName) != nullptr)
                                            {
                                                parameterTypes.emplace_back(paramName, value::ParameterType::forInterface(typeName));
                                            }
                                            else if (environment->findClass(typeName) != nullptr)
                                            {
                                                parameterTypes.emplace_back(paramName, value::ParameterType::forClass(typeName));
                                            }
                                            else
                                            {
                                                parameterTypes.emplace_back(paramName, value::ParameterType(baseType));
                                            }
                                        }
                                        else
                                        {
                                            parameterTypes.emplace_back(paramName, value::ParameterType(baseType));
                                        }
                                    }
                                }

                                // Get return type from generic return type
                                value::ValueType returnType = value::ValueType::VOID;
                                if (auto genericReturnType = methodNode->getGenericReturnType())
                                {
                                    if (genericReturnType->isGenericParameter())
                                    {
                                        // For generic return types (T, E, etc.), use OBJECT as the base type
                                        returnType = value::ValueType::OBJECT;
                                    }
                                    else
                                    {
                                        returnType = genericReturnType->getConcreteType();
                                    }
                                }

                                // Register the method definition directly as a callable function
                                auto funcDef = std::make_shared<runtimeTypes::global::FunctionDefinition>(
                                    qualifiedName,
                                    returnType,
                                    parameterTypes
                                );

                                // Copy the body from the static method definition
                                funcDef->setBody(method->getBodyPtr());

                                funcRegistry->registerFunction(qualifiedName, funcDef);
                            }
                        }
                    }
                }
            }
        }
    }
}
