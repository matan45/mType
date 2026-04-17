#include "FunctionRegistrar.hpp"
#include "AnnotationRetention.hpp"
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ImportNode.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../validation/AnnotationValidator.hpp"
#include "../../../validation/AnnotationUsageValidator.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/DuplicateSignatureException.hpp"
#include "../../../runtimeTypes/klass/SignatureUtils.hpp"
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
                // Check if this arg is a declared type parameter (T, K, V, etc.)
                // Don't use isGenericParameter() as it returns true for all string baseTypes (including class names)
                // Instead, check if the base name is in declaredTypeParams
                std::string argName = arg->getGenericName();
                if (declaredTypeParams.find(argName) != declaredTypeParams.end() && !arg->isParameterized())
                {
                    // This is a simple type parameter like "T"
                    usedParams.insert(argName);
                }
                else
                {
                    // Either a concrete type (String, Int) or a parameterized type (Wrapper<T>)
                    // Recurse to extract nested type parameters
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

        // Check for duplicate signature (overloading by name is now allowed, but signatures must be unique)
        // Get the function's parameter types for signature comparison
        auto paramTypesVec = functionNode->getParameterTypes();
        std::vector<std::pair<std::string, value::ParameterType>> params;
        for (const auto& param : paramTypesVec) {
            params.emplace_back(param.first, param.second);
        }

        // Check in environment's function registry for existing functions with same name.
        // If the exact signature is already registered, this is an idempotent re-import
        // (the same AST traversed via two import chains) — silently skip, matching the
        // behavior of ClassRegistrar::registerSingleClass.
        auto funcRegistry = environment->getFunctionRegistry();
        if (funcRegistry) {
            auto existingFunc = funcRegistry->findFunctionBySignature(funcName, params);
            if (existingFunc) {
                return;
            }
        }

        // Extract parameter information (preserves class names like "Pair<K, V>")
        // paramTypesVec already retrieved above
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

        // MYT-110: function-level annotations into bytecode metadata (typed-args),
        // honoring @Retention(SOURCE) filter.
        for (const auto& annotationNode : functionNode->getAnnotations())
        {
            if (!annotationNode) continue;
            if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
            bytecode::BytecodeProgram::AnnotationData annot;
            populateAnnotationData(annot, *annotationNode);
            metadata.annotations.push_back(std::move(annot));
        }

        // MYT-110: per-parameter annotations into bytecode metadata.
        {
            const auto& astParamAnnots = functionNode->getParameterAnnotations();
            metadata.parameterAnnotations.resize(paramNames.size());
            for (size_t pi = 0; pi < paramNames.size(); ++pi)
            {
                if (pi >= astParamAnnots.size()) continue;
                for (const auto& annotationNode : astParamAnnots[pi])
                {
                    if (!annotationNode) continue;
                    if (!shouldRetainAnnotation(*annotationNode, environment)) continue;
                    bytecode::BytecodeProgram::AnnotationData annot;
                    populateAnnotationData(annot, *annotationNode);
                    metadata.parameterAnnotations[pi].push_back(std::move(annot));
                }
            }
        }

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

        // Generate mangled name for overload resolution
        std::string typeSignature = runtimeTypes::klass::SignatureUtils::generateTypeSignature(params);
        std::string mangledName;
        if (typeSignature.empty()) {
            mangledName = funcName;  // No parameters - use plain name
        } else {
            mangledName = funcName + "/" + typeSignature;  // With parameters - add signature
        }

        // Create FunctionDefinition for FunctionRegistry
        // For return class name, use returnTypeStr only if return type is OBJECT
        std::string returnClassName = (returnType == value::ValueType::OBJECT) ? returnTypeStr : "";
        auto funcDef = std::make_shared<runtimeTypes::global::FunctionDefinition>(
            funcName,
            returnType,
            returnClassName,
            params
        );
        funcDef->setIsAsync(functionNode->getIsAsync());

        // Store generic type parameters if the function is generic
        if (functionNode->isGeneric())
        {
            funcDef->setGenericTypeParameters(functionNode->getGenericTypeParameters());
        }

        // MYT-110: copy function-level and per-parameter annotations to the
        // runtime definition, honoring @Retention(SOURCE) filter.
        for (const auto& annotation : functionNode->getAnnotations())
        {
            if (!annotation) continue;
            if (!shouldRetainAnnotation(*annotation, environment)) continue;
            funcDef->addAnnotation(annotation);
        }
        {
            const auto& astParamAnnots = functionNode->getParameterAnnotations();
            std::vector<std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>>> filtered;
            filtered.reserve(astParamAnnots.size());
            for (const auto& perParam : astParamAnnots)
            {
                std::vector<std::shared_ptr<ast::nodes::annotations::AnnotationNode>> kept;
                for (const auto& a : perParam)
                {
                    if (!a) continue;
                    if (!shouldRetainAnnotation(*a, environment)) continue;
                    kept.push_back(a);
                }
                filtered.push_back(std::move(kept));
            }
            funcDef->setParameterAnnotations(std::move(filtered));
        }

        // Register in FunctionRegistry for overload tracking
        if (funcRegistry)
        {
            funcRegistry->registerFunction(funcName, funcDef);
        }

        // Register the function signature with BOTH original name and mangled name
        metadata.mangledName = mangledName;
        program.registerFunction(funcName, metadata);          // Original name for tracking
        program.registerFunction(mangledName, metadata);       // Mangled name for VM lookup
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
        // MYT-108: run the typed usage validator over every annotation on the
        // top-level function first (catches unknown annotations, bad types,
        // missing required params, fills in defaults).
        for (const auto& annotation : functionNode->getAnnotations())
        {
            if (!annotation) continue;
            ::validation::AnnotationUsageValidator::validate(
                annotation, environment, annotation->getLocation(),
                ::validation::AnnotationHostKind::FUNCTION);
        }

        // MYT-110: validate per-parameter annotations with PARAMETER host kind
        for (const auto& perParam : functionNode->getParameterAnnotations())
        {
            for (const auto& ann : perParam)
            {
                if (!ann) continue;
                ::validation::AnnotationUsageValidator::validate(
                    ann, environment, ann->getLocation(),
                    ::validation::AnnotationHostKind::PARAMETER);
            }
        }

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
