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
#include <filesystem>
#include <iostream>

namespace services
{
    bool Runtime::execute(const std::string& compiledFile)
    {
        try
        {
            // Deserialize the cached AST
            ast::serialization::ASTDeserializer deserializer;
            auto ast = deserializer.deserialize(compiledFile);

            if (!ast)
            {
                return false;
            }

            // Create completely isolated execution environment
            environment::EnvironmentBuilder envBuilder;
            auto isolatedEnvironment = envBuilder.build();
            auto isolatedEvaluator = std::make_unique<evaluator::Evaluator>(isolatedEnvironment);

            // Create fresh ImportManager for this execution
            auto importManager = std::make_unique<ImportManager>();

            // Set base directory to the directory of the compiled file
            std::filesystem::path compiledPath(compiledFile);
            importManager->setBaseDirectory(compiledPath.parent_path().string());

            // Set ImportManager on isolated environment
            ImportManager* importManagerPtr = importManager.release();
            isolatedEnvironment->setImportManager(importManagerPtr);

            // Pre-register all class definitions in isolated environment
            preRegisterClassDefinitions(ast.get(), isolatedEvaluator.get());

            // Execute the cached AST with completely isolated evaluator
            isolatedEvaluator->evaluate(ast.get());

            return true;
        }
        catch (const std::exception&)
        {
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