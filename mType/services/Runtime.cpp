#include "Runtime.hpp"
#include "../ast/serialization/ASTDeserializer.hpp"
#include "../environment/Environment.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../evaluator/Evaluator.hpp"
#include "ImportManager.hpp"
#include "../ast/ASTNode.hpp"
#include "../ast/nodes/classes/ClassNode.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../environment/registry/NativeRegistry.hpp"
#include <filesystem>
#include <iostream>

namespace services
{
    bool Runtime::execute(const std::string& compiledFile)
    {
        try
        {
            std::cout << "[DEBUG] Runtime: Starting execution of " << compiledFile << std::endl;

            // Deserialize the cached AST
            ast::serialization::ASTDeserializer deserializer;
            auto ast = deserializer.deserialize(compiledFile);

            if (!ast)
            {
                std::cout << "[ERROR] Runtime: Failed to deserialize AST from " << compiledFile << std::endl;
                return false;
            }

            std::cout << "[DEBUG] Runtime: AST deserialized successfully" << std::endl;

            // Create completely isolated execution environment
            environment::EnvironmentBuilder envBuilder;
            auto isolatedEnvironment = envBuilder.build();
            auto isolatedEvaluator = std::make_unique<evaluator::Evaluator>(isolatedEnvironment);

            // Set up method call handler for native functions
            auto nativeRegistry = isolatedEnvironment->getNativeRegistry();
            if (nativeRegistry) {
                nativeRegistry->setMethodCallHandler(
                    [&isolatedEvaluator](std::shared_ptr<runtimeTypes::klass::ObjectInstance> instance,
                                        const std::string& methodName,
                                        const std::vector<value::Value>& args) -> value::Value {
                        return isolatedEvaluator->callMethodOnInstance(instance, methodName, args);
                    }
                );
            }

            // Create fresh ImportManager for this execution
            auto importManager = std::make_unique<ImportManager>();

            // Set base directory to the release directory for cached execution
            std::filesystem::path compiledPath(compiledFile);
            std::string releaseDir = "release";
            if (std::filesystem::exists(releaseDir)) {
                importManager->setBaseDirectory(releaseDir);
            } else {
                // Fallback to the directory of the compiled file
                importManager->setBaseDirectory(compiledPath.parent_path().string());
            }

            // Set ImportManager on isolated environment
            ImportManager* importManagerPtr = importManager.release();
            isolatedEnvironment->setImportManager(importManagerPtr);

            // Pre-register all class definitions in isolated environment
            preRegisterClassDefinitions(ast.get(), isolatedEvaluator.get());

            std::cout << "[DEBUG] Runtime: About to execute AST" << std::endl;

            // Execute the cached AST with completely isolated evaluator
            isolatedEvaluator->evaluate(ast.get());

            std::cout << "[DEBUG] Runtime: Execution completed successfully" << std::endl;
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[ERROR] Runtime error: " << e.what() << std::endl;
            return false;
        }
    }

    void Runtime::preRegisterClassDefinitions(ast::ASTNode* node, evaluator::Evaluator* evaluator)
    {
        if (!node || !evaluator) return;

        // Check if this node is a ClassNode
        if (auto classNode = dynamic_cast<ast::nodes::classes::ClassNode*>(node))
        {
            // Pre-register class in isolated environment
            evaluator->evaluate(classNode);
            return; // No need to traverse children of ClassNode
        }

        // Recursively process child nodes
        if (auto programNode = dynamic_cast<ast::nodes::statements::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                preRegisterClassDefinitions(statement.get(), evaluator);
            }
        }
        else if (auto blockNode = dynamic_cast<ast::nodes::statements::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                preRegisterClassDefinitions(statement.get(), evaluator);
            }
        }
        // Add other node types that may contain ClassNodes as needed
    }
}