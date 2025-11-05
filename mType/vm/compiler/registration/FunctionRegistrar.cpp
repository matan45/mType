#include "FunctionRegistrar.hpp"
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ImportNode.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../validation/AnnotationValidator.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../errors/TypeException.hpp"
#include <stdexcept>

namespace vm::compiler::registration
{
    FunctionRegistrar::FunctionRegistrar(
        std::shared_ptr<environment::Environment> env,
        bytecode::BytecodeProgram& prog)
        : environment(env)
        , program(prog)
    {
    }

    void FunctionRegistrar::registerFunctionSignatures(ast::ASTNode* node)
    {
        if (!node) return;

        // Check if this node is a FunctionNode
        if (auto functionNode = dynamic_cast<ast::FunctionNode*>(node))
        {
            registerSingleFunction(functionNode);
            return; // Don't traverse into function body
        }

        // Check if this node is a ClassNode - skip it (methods handled by ClassRegistrar)
        if (dynamic_cast<ast::ClassNode*>(node))
        {
            return; // ClassRegistrar handles methods
        }

        // Recursively process child nodes
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                registerFunctionSignatures(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                registerFunctionSignatures(statement.get());
            }
        }
        else if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            // Process functions from imported AST
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                registerFunctionSignatures(importNode->getImportedAST());
            }
        }
    }

    void FunctionRegistrar::registerSingleFunction(ast::FunctionNode* functionNode)
    {
        std::string funcName = functionNode->getName();

        // Check if already registered
        const auto* existingFunc = program.getFunction(funcName);
        if (existingFunc)
        {
            // Allow native functions to be skipped, but reject user-defined duplicates
            if (!existingFunc->isNative)
            {
                throw errors::TypeException(
                    "Function overloading is not supported. Global function '" + funcName + "' is already defined",
                    functionNode->getLocation()
                );
            }
            return; // Native function already registered, skip
        }

        // Extract parameter information
        auto paramTypesVec = functionNode->getParameterTypes();
        std::vector<std::string> paramNames;
        std::vector<std::string> paramTypes;

        for (const auto& param : paramTypesVec)
        {
            paramNames.push_back(param.first);
            const auto& paramType = param.second;
            if (paramType.basicType == value::ValueType::OBJECT && paramType.className.has_value())
            {
                paramTypes.push_back(paramType.className.value());
            }
            else
            {
                paramTypes.push_back(::types::TypeConversionUtils::getTypeDisplayName(paramType.basicType));
            }
        }

        // Get return type
        value::ValueType returnType = functionNode->getReturnType();
        std::string returnTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(returnType);

        // Create placeholder metadata (will be filled in during actual compilation)
        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = funcName;
        metadata.startOffset = 0; // Placeholder
        metadata.instructionCount = 0; // Placeholder
        metadata.localCount = 0; // Placeholder
        metadata.parameterCount = paramNames.size();
        metadata.parameterNames = paramNames;
        metadata.parameterTypes = paramTypes;
        metadata.returnType = returnTypeStr;
        metadata.isStatic = false;
        metadata.isNative = false;
        metadata.isAsync = functionNode->getIsAsync();

        // Store generic type parameters if the function is generic
        if (functionNode->isGeneric())
        {
            const auto& genericParams = functionNode->getGenericTypeParameters();
            for (const auto& param : genericParams)
            {
                metadata.genericTypeParameters.push_back(param.name);
            }
        }

        // NOTE: @Throw annotation validation is deferred until after classes are registered
        // See validateThrowAnnotations() method

        // Register the function signature
        program.registerFunction(funcName, metadata);
    }

    void FunctionRegistrar::validateThrowAnnotations(ast::ASTNode* node)
    {
        if (!node) return;

        // Check if this node is a FunctionNode
        if (auto functionNode = dynamic_cast<ast::FunctionNode*>(node))
        {
            validateSingleFunctionThrow(functionNode);
            return; // Don't traverse into function body
        }

        // Check if this node is a ClassNode - skip it (methods handled by ClassRegistrar)
        if (dynamic_cast<ast::ClassNode*>(node))
        {
            return; // ClassRegistrar handles methods
        }

        // Recursively process child nodes
        if (auto programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& statement : programNode->getStatements())
            {
                validateThrowAnnotations(statement.get());
            }
        }
        else if (auto blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& statement : blockNode->getStatements())
            {
                validateThrowAnnotations(statement.get());
            }
        }
        else if (auto importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            // Process functions from imported AST
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                validateThrowAnnotations(importNode->getImportedAST());
            }
        }
    }

    void FunctionRegistrar::validateSingleFunctionThrow(ast::FunctionNode* functionNode)
    {
        // Validate @Throw annotation if present
        if (auto throwAnnotation = functionNode->getAnnotation("Throw"))
        {
            ::validation::AnnotationValidator::validateThrowAnnotation(
                throwAnnotation,
                environment,
                functionNode->getLocation()
            );
        }
    }
}
