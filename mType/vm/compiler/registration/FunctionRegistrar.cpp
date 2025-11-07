#include "FunctionRegistrar.hpp"
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ImportNode.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../validation/AnnotationValidator.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../errors/TypeException.hpp"
#include "../types/GenericPatternAnalyzer.hpp"
#include <stdexcept>

namespace vm::compiler::registration
{
    // PHASE 2: Helper function to recursively extract type parameters from GenericType
    static void extractTypeParametersFromGenericType(
        const std::shared_ptr<ast::GenericType>& type,
        const std::unordered_set<std::string>& declaredTypeParams,
        std::unordered_set<std::string>& usedParams)
    {
        if (!type) return;

        // IMPORTANT: Check if parameterized FIRST, before checking if generic parameter
        // "Pair<K, V>" is both a generic parameter (baseType is string) AND parameterized (has type args)
        // We need to process the type arguments to extract K and V
        if (type->isParameterized())
        {
            const auto& typeArgs = type->getTypeArguments();
            for (const auto& arg : typeArgs)
            {
                if (arg->isGenericParameter())
                {
                    // This is a nested type parameter!
                    std::string paramName = arg->getGenericName();
                    if (declaredTypeParams.find(paramName) != declaredTypeParams.end())
                    {
                        usedParams.insert(paramName);
                    }
                }
                else
                {
                    // Recurse deeper (e.g., List<Box<T>>)
                    extractTypeParametersFromGenericType(arg, declaredTypeParams, usedParams);
                }
            }
        }
        // If it's a simple type parameter like "T" (not parameterized), don't count it
        // Only nested parameters (inside type arguments) count
    }

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

        // Extract parameter information (preserves class names like "Pair<K, V>")
        auto paramTypesVec = functionNode->getParameterTypes();
        std::vector<std::string> paramNames;
        std::vector<std::string> paramTypes;

        for (const auto& param : paramTypesVec)
        {
            paramNames.push_back(param.first);
            const auto& paramType = param.second;

            // PHASE 2 FIX: Use ParameterType::toString() to preserve generic type info
            // This returns the full type string: "T", "Pair<K, V>", "Box<T>", etc.
            paramTypes.push_back(paramType.toString());
        }

        // Get return type with class name preservation
        std::string returnTypeStr;
        value::ValueType returnType = functionNode->getReturnType();

        // PHASE 3 FIX: Use GenericReturnType to preserve full type information
        if (auto genericReturnType = functionNode->getGenericReturnType())
        {
            if (genericReturnType->isGenericParameter())
            {
                // Type parameter like "T" or class name like "Pair" or "Pair<K, V>"
                std::string typeName = genericReturnType->getGenericName();
                if (genericReturnType->isParameterized())
                {
                    // Has type arguments like "Pair<K, V>" - use toString()
                    returnTypeStr = genericReturnType->toString();
                }
                else
                {
                    // No type arguments - just use the name ("T" or "Pair")
                    returnTypeStr = typeName;
                }
            }
            else if (returnType == value::ValueType::OBJECT && genericReturnType->isParameterized())
            {
                // Concrete type with type arguments - use toString()
                returnTypeStr = genericReturnType->toString();
            }
            else
            {
                // Primitive or simple type - use basic type name
                returnTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(returnType);
            }
        }
        else
        {
            // Fallback for legacy code
            returnTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(returnType);
        }

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

            // PHASE 2: Analyze parameter types for nested generic inference
            // Build set of declared type parameters for quick lookup
            std::unordered_set<std::string> declaredTypeParams;
            for (const auto& param : genericParams)
            {
                declaredTypeParams.insert(param.name);
            }

            // PHASE 2 FIX: Directly analyze GenericType objects instead of strings
            // This is more robust than parsing string representations
            const auto& genericParamTypes = functionNode->getGenericParameters();
            for (size_t i = 0; i < genericParamTypes.size(); ++i)
            {
                const auto& paramPair = genericParamTypes[i];
                std::unordered_set<std::string> usedParams;
                if (paramPair.second)
                {
                    extractTypeParametersFromGenericType(paramPair.second, declaredTypeParams, usedParams);
                }
                metadata.parameterTypeParameterUsage.push_back(usedParams);
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
