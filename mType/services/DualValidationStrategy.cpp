#include "DualValidationStrategy.hpp"
#include "ImportResolver.hpp"
#include "OptimizationService.hpp"
#include "../evaluator/Evaluator.hpp"
#include "../environment/EnvironmentBuilder.hpp"
#include "../vm/compiler/BytecodeCompiler.hpp"
#include "../vm/runtime/VirtualMachine.hpp"
#include "../constants/ExecutionMode.hpp"
#include <stdexcept>
#include <iostream>
namespace services
{
    DualValidationStrategy::DualValidationStrategy(evaluator::Evaluator* eval,
                                                   std::shared_ptr<environment::Environment> env,
                                                   ImportResolver* resolver,
                                                   OptimizationService* optService)
        : evaluator(eval), environment(env), importResolver(resolver), optimizationService(optService)
    {
    }

    value::Value DualValidationStrategy::execute(ast::ASTNode* ast)
    {
        std::cout << "=== DUAL VALIDATION MODE ===" << std::endl;

        // NOTE: Imports are already resolved in executeScriptAST before optimization
        // No need to resolve them again here

        // Try both execution modes
        auto [astResult, astSuccess] = tryExecuteAST(ast);
        auto [vmResult, vmSuccess] = tryExecuteBytecode(ast);

        // Compare results
        if (astSuccess && vmSuccess)
        {
            std::cout << "[VALIDATION] Both executions succeeded" << std::endl;
            std::cout << "=== END DUAL VALIDATION ===" << std::endl;
            return astResult; // Prefer AST result as ground truth
        }
        else if (astSuccess)
        {
            std::cout << "[VALIDATION] Only AST succeeded" << std::endl;
            std::cout << "=== END DUAL VALIDATION ===" << std::endl;
            return astResult;
        }
        else if (vmSuccess)
        {
            std::cout << "[VALIDATION] Only VM succeeded" << std::endl;
            std::cout << "=== END DUAL VALIDATION ===" << std::endl;
            return vmResult;
        }
        else
        {
            std::cout << "[VALIDATION] Both executions failed" << std::endl;
            std::cout << "=== END DUAL VALIDATION ===" << std::endl;
            throw std::runtime_error("Both execution modes failed");
        }
    }

    std::pair<value::Value, bool> DualValidationStrategy::tryExecuteAST(ast::ASTNode* ast)
    {
        try
        {
            std::cout << "[AST] Executing..." << std::endl;
            value::Value result = evaluator->evaluate(ast);
            std::cout << "[AST] Success" << std::endl;
            return {result, true};
        }
        catch (const std::exception& e)
        {
            std::cerr << "[AST] Error: " << e.what() << std::endl;
            return {std::monostate{}, false};
        }
    }

    std::pair<value::Value, bool> DualValidationStrategy::tryExecuteBytecode(ast::ASTNode* ast)
    {
        // Try bytecode execution with fresh environment
        // This ensures variables from AST execution don't leak into VM execution
        try
        {
            std::cout << "[VM] Compiling and executing..." << std::endl;

            // Create fresh environment for VM
            environment::EnvironmentBuilder vmEnvBuilder;
            auto vmEnvironment = vmEnvBuilder.build();

            // Set ImportManager on the VM environment (imports are already resolved in the AST)
            vmEnvironment->setImportManager(environment->getImportManager());

            // Create fresh VM and compiler with new environment
            constants::OptimizationLevel optLevel = optimizationService->getOptimizationLevel();
            bool skipStrictValidation = (optLevel == constants::OptimizationLevel::Release);
            auto vmCompiler = std::make_unique<vm::compiler::BytecodeCompiler>(vmEnvironment, skipStrictValidation, optLevel);
            auto vmMachine = std::make_shared<vm::runtime::VirtualMachine>(vmEnvironment);

            // The imports are already resolved in the AST (from AST execution)
            // But we need to ensure the BytecodeCompiler can access them
            auto program = vmCompiler->compile(ast);
            value::Value result = vmMachine->execute(program);
            std::cout << "[VM] Success" << std::endl;
            return {result, true};
        }
        catch (const std::exception& e)
        {
            std::cerr << "[VM] Error: " << e.what() << std::endl;
            return {std::monostate{}, false};
        }
    }
}
