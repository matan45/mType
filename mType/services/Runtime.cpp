#include "Runtime.hpp"
#include "../ast/serialization/ASTDeserializer.hpp"
#include "../ast/serialization/CompleteJSONDeserializer.hpp"
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

            // Check file extension to determine deserializer type
            std::filesystem::path filePath(compiledFile);
            std::string extension = filePath.extension().string();

            std::shared_ptr<ast::ASTNode> ast;

            if (extension == ".json")
            {
                // Use JSON deserializer for .json files
                // IMPORTANT: Keep deserializer alive to preserve import cache and prevent dangling pointers
                static ast::serialization::CompleteJSONDeserializer jsonDeserializer;
                ast = jsonDeserializer.deserializeFromFile(compiledFile);
            }
            else
            {
                // Use binary deserializer for .mtc files
                ast::serialization::ASTDeserializer deserializer;
                auto uniqueAst = deserializer.deserialize(compiledFile);
                ast = std::shared_ptr<ast::ASTNode>(uniqueAst.release());
            }

            if (!ast)
            {
                std::cout << "[ERROR] Runtime: Failed to deserialize AST from " << compiledFile << std::endl;
                return false;
            }


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

            // For JSON files, imports are already resolved and embedded in the AST
            // For binary files, we still need an ImportManager for runtime resolution
            if (extension != ".json")
            {
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
            }
            else
            {
            }

            // Pre-register all class definitions in isolated environment
            preRegisterClassDefinitions(ast.get(), isolatedEvaluator.get());


            // Execute the cached AST with completely isolated evaluator
            isolatedEvaluator->evaluate(ast.get());

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