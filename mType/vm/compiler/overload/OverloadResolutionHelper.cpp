#include "OverloadResolutionHelper.hpp"
#include "OverloadResolver.hpp"
#include "../../../runtimeTypes/klass/SignatureUtils.hpp"
#include "../../../errors/NoMatchingOverloadException.hpp"
#include "../../../errors/AmbiguousCallException.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../types/TypeRegistry.hpp"
#include <iostream>

namespace vm::compiler::overload
{
    OverloadResolutionHelper::OverloadResolutionHelper(visitors::CompilerContext& context)
        : ctx(context)
    {
    }

    std::vector<value::ParameterType> OverloadResolutionHelper::inferArgumentTypes(
        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        std::vector<value::ParameterType> argTypes;
        argTypes.reserve(arguments.size());

        for (const auto& arg : arguments)
        {
            // Infer the basic value type
            value::ValueType basicType = ctx.typeInference.inferExpressionType(arg.get());

            if (basicType == value::ValueType::OBJECT)
            {
                // For objects, try to infer the class name
                std::string className = ctx.typeInference.inferExpressionClassName(arg.get());

                if (!className.empty())
                {
                    // Check if it's an interface or class
                    auto interfaceDef = ctx.environment->findInterface(className);
                    if (interfaceDef)
                    {
                        argTypes.emplace_back(value::ParameterType::forInterface(className));
                    }
                    else
                    {
                        argTypes.emplace_back(value::ParameterType::forClass(className));
                    }
                }
                else
                {
                    // Unknown object type
                    argTypes.emplace_back(value::ParameterType(basicType));
                }
            }
            else
            {
                // Primitive type
                argTypes.emplace_back(value::ParameterType(basicType));
            }
        }

        return argTypes;
    }

    std::string OverloadResolutionHelper::buildMangledMethodName(
        const std::string& className,
        const std::string& methodName,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters)
    {
        // Use SignatureUtils utility to build qualified name with signature
        return runtimeTypes::klass::SignatureUtils::buildQualifiedName(className, methodName, parameters);
    }

    std::string OverloadResolutionHelper::buildMangledFunctionName(
        const std::string& functionName,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters)
    {
        // Build function signature: "functionName/Type1,Type2,..."
        std::string signature = runtimeTypes::klass::SignatureUtils::generateTypeSignature(parameters);
        if (signature.empty())
        {
            return functionName; // No parameters
        }
        return functionName + "/" + signature;
    }

    std::string OverloadResolutionHelper::resolveInstanceMethodOverload(
        const std::string& className,
        const std::string& methodName,
        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
        const ast::SourceLocation& location,
        bool hasGenericTypeArgs,
        size_t genericTypeArgCount)
    {
        // Extract base class name (without generic parameters)
        std::string baseClassName = className;
        size_t anglePos = className.find('<');
        if (anglePos != std::string::npos)
        {
            baseClassName = className.substr(0, anglePos);
        }

        // Get class definition
        auto classDef = ctx.environment->findClass(baseClassName);
        if (!classDef)
        {
            // Class not found - return plain method name (runtime will handle error)
            return methodName;
        }

        // Get all instance method overloads
        auto overloads = classDef->getAllInstanceMethodOverloads(methodName);

        if (overloads.empty())
        {
            // No methods found - return plain name (runtime will handle error)
            return methodName;
        }

        // Filter overloads based on generic type arguments (Java/C# style)
        std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>> filteredOverloads;

        if (hasGenericTypeArgs)
        {
            // User explicitly provided type arguments: method<Type>()
            // Only consider generic methods with matching number of generic type parameters
            for (const auto& overload : overloads)
            {
                size_t methodGenericParamCount = overload->getGenericTypeParameters().size();
                if (methodGenericParamCount == genericTypeArgCount)
                {
                    filteredOverloads.push_back(overload);
                }
            }
        }
        else
        {
            // NO explicit type arguments provided
            // Java/C# rule: Prefer non-generic overloads over generic ones
            std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>> nonGenericOverloads;
            std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>> genericOverloads;

            for (const auto& overload : overloads)
            {
                size_t genericParamCount = overload->getGenericTypeParameters().size();
                const auto& genericParams = overload->getGenericParameters();

                // Check parameter types using GenericType to see if they contain unresolved generics
                bool hasUnresolvedGenerics = false;
                for (const auto& [paramName, genericType] : genericParams) {
                    std::string typeStr = genericType->toString();

                    // Check if this type contains unresolved generic parameters (like T, K, V)
                    // Look for patterns like "T", "Box<T>", "List<K>", etc.
                    auto& registry = ::types::getGlobalTypeRegistry();

                    // Simple check: if type string is a single capital letter and not in registry
                    if (typeStr.length() == 1 && std::isupper(typeStr[0]) && !registry.hasType(typeStr)) {
                        hasUnresolvedGenerics = true;
                    }
                    // Check for parameterized types with generics: Box<T>, List<K>, etc.
                    else if (typeStr.find('<') != std::string::npos) {
                        size_t start = typeStr.find('<');
                        size_t end = typeStr.find('>');
                        if (end != std::string::npos && end > start) {
                            std::string typeArg = typeStr.substr(start + 1, end - start - 1);
                            // Trim spaces
                            typeArg.erase(0, typeArg.find_first_not_of(" \t"));
                            typeArg.erase(typeArg.find_last_not_of(" \t") + 1);

                            // Check if type argument is unresolved (single letter not in registry)
                            if (typeArg.length() == 1 && std::isupper(typeArg[0]) && !registry.hasType(typeArg)) {
                                hasUnresolvedGenerics = true;
                            }
                        }
                    }
                }

                if (!hasUnresolvedGenerics)
                {
                    // Non-generic method (all types are concrete)
                    nonGenericOverloads.push_back(overload);
                }
                else
                {
                    // Generic method (contains unresolved type parameters)
                    genericOverloads.push_back(overload);
                }
            }

            // If we have non-generic overloads, use only those
            // Otherwise, fall back to generic overloads
            if (!nonGenericOverloads.empty())
            {
                filteredOverloads = nonGenericOverloads;
            }
            else
            {
                filteredOverloads = genericOverloads;
            }
        }

        if (filteredOverloads.empty())
        {
            // No matching overloads after filtering
            return methodName;
        }

        if (filteredOverloads.size() == 1)
        {
            // Only one overload after filtering - return mangled name
            // Use genericParameters to preserve full type signatures (e.g., UnaryFunction<T>)
            // Skip 'this' parameter for instance methods (genericParameters doesn't include it)
            const auto& method = filteredOverloads[0];
            const auto& genericParams = method->getGenericParameters();

            // Build type signature using GenericType::toString() to preserve generics
            std::vector<std::string> typeNames;
            typeNames.reserve(genericParams.size());
            for (const auto& [paramName, genericType] : genericParams) {
                typeNames.push_back(genericType->toString());
            }

            std::string typeSignature = runtimeTypes::klass::SignatureUtils::generateTypeSignatureFromNames(typeNames);
            std::string mangledName = runtimeTypes::klass::SignatureUtils::buildQualifiedName(baseClassName, methodName, typeSignature);
            return mangledName;
        }

        // Multiple overloads - need resolution
        std::vector<value::ParameterType> argTypes = inferArgumentTypes(arguments);

        // Use OverloadResolver to find best match (using filtered overloads)
        auto result = OverloadResolver::resolveMethodOverload(filteredOverloads, argTypes, location);

        if (result.isAmbiguous)
        {
            // Build argument type strings
            std::vector<std::string> argTypeStrings;
            for (const auto& argType : argTypes)
            {
                argTypeStrings.push_back(runtimeTypes::klass::SignatureUtils::getTypeName(argType));
            }

            // Build candidate signature strings
            std::vector<std::string> candidateSignatures;
            for (const auto& candidate : result.ambiguousCandidates)
            {
                candidateSignatures.push_back(
                    runtimeTypes::klass::SignatureUtils::generateTypeSignature(candidate->getParametersWithTypes())
                );
            }

            throw errors::AmbiguousCallException(
                baseClassName + "::" + methodName,
                candidateSignatures,
                argTypeStrings,
                location
            );
        }

        if (!result.hasViableCandidates || !result.selectedOverload)
        {
            // Build argument type strings
            std::vector<std::string> argTypeStrings;
            for (const auto& argType : argTypes)
            {
                argTypeStrings.push_back(runtimeTypes::klass::SignatureUtils::getTypeName(argType));
            }

            // Build available signature strings (from filtered overloads)
            std::vector<std::string> availableSignatures;
            for (const auto& overload : filteredOverloads)
            {
                availableSignatures.push_back(
                    runtimeTypes::klass::SignatureUtils::generateTypeSignature(overload->getParametersWithTypes())
                );
            }

            throw errors::NoMatchingOverloadException(
                baseClassName + "::" + methodName,
                availableSignatures,
                argTypeStrings,
                location
            );
        }

        // Successfully resolved
        // Use genericParameters to preserve full type signatures (e.g., UnaryFunction<T>)
        // Skip 'this' parameter for instance methods (genericParameters doesn't include it)
        const auto& genericParams = result.selectedOverload->getGenericParameters();

        // Build type signature using GenericType::toString() to preserve generics
        std::vector<std::string> typeNames;
        typeNames.reserve(genericParams.size());
        for (const auto& [paramName, genericType] : genericParams) {
            std::string typeName = genericType->toString();
            typeNames.push_back(typeName);
        }

        std::string typeSignature = runtimeTypes::klass::SignatureUtils::generateTypeSignatureFromNames(typeNames);
        std::string resolvedName = runtimeTypes::klass::SignatureUtils::buildQualifiedName(baseClassName, methodName, typeSignature);
        return resolvedName;
    }

    std::string OverloadResolutionHelper::resolveStaticMethodOverload(
        const std::string& className,
        const std::string& methodName,
        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
        const ast::SourceLocation& location)
    {
        // Extract base class name (without generic parameters)
        std::string baseClassName = className;
        size_t anglePos = className.find('<');
        if (anglePos != std::string::npos)
        {
            baseClassName = className.substr(0, anglePos);
        }

        // Get class definition
        auto classDef = ctx.environment->findClass(baseClassName);
        if (!classDef)
        {
            // Class not found - return qualified plain name
            return baseClassName + "::" + methodName;
        }

        // Get all static method overloads
        auto overloads = classDef->getAllStaticMethodOverloads(methodName);
        if (overloads.empty())
        {
            // No methods found - return qualified plain name
            return baseClassName + "::" + methodName;
        }

        if (overloads.size() == 1)
        {
            // Only one overload - return mangled name with $static suffix
            // Use genericParameters to preserve full type signatures (e.g., UnaryFunction<T>)
            const auto& method = overloads[0];
            const auto& genericParams = method->getGenericParameters();

            // Build type signature using GenericType::toString() to preserve generics
            std::vector<std::string> typeNames;
            typeNames.reserve(genericParams.size());
            for (const auto& [paramName, genericType] : genericParams) {
                typeNames.push_back(genericType->toString());
            }

            std::string typeSignature = runtimeTypes::klass::SignatureUtils::generateTypeSignatureFromNames(typeNames);
            std::string mangledName = runtimeTypes::klass::SignatureUtils::buildQualifiedName(baseClassName, methodName, typeSignature);
            return mangledName + "$static";
        }

        // For static methods, no filtering needed (yet) - use all overloads
        auto filteredOverloads = overloads;

        // Multiple overloads - need resolution
        std::vector<value::ParameterType> argTypes = inferArgumentTypes(arguments);

        // Use OverloadResolver to find best match
        auto result = OverloadResolver::resolveMethodOverload(filteredOverloads, argTypes, location);

        if (result.isAmbiguous)
        {
            // Build argument type strings
            std::vector<std::string> argTypeStrings;
            for (const auto& argType : argTypes)
            {
                argTypeStrings.push_back(runtimeTypes::klass::SignatureUtils::getTypeName(argType));
            }

            // Build candidate signature strings
            std::vector<std::string> candidateSignatures;
            for (const auto& candidate : result.ambiguousCandidates)
            {
                candidateSignatures.push_back(
                    runtimeTypes::klass::SignatureUtils::generateTypeSignature(candidate->getParametersWithTypes())
                );
            }

            throw errors::AmbiguousCallException(
                baseClassName + "::" + methodName,
                candidateSignatures,
                argTypeStrings,
                location
            );
        }

        if (!result.hasViableCandidates || !result.selectedOverload)
        {
            // Build argument type strings
            std::vector<std::string> argTypeStrings;
            for (const auto& argType : argTypes)
            {
                argTypeStrings.push_back(runtimeTypes::klass::SignatureUtils::getTypeName(argType));
            }

            // Build available signature strings (from filtered overloads)
            std::vector<std::string> availableSignatures;
            for (const auto& overload : filteredOverloads)
            {
                availableSignatures.push_back(
                    runtimeTypes::klass::SignatureUtils::generateTypeSignature(overload->getParametersWithTypes())
                );
            }

            throw errors::NoMatchingOverloadException(
                baseClassName + "::" + methodName,
                availableSignatures,
                argTypeStrings,
                location
            );
        }

        // Successfully resolved - add $static suffix for static methods
        // Use genericParameters to preserve full type signatures (e.g., UnaryFunction<T>)
        const auto& genericParams = result.selectedOverload->getGenericParameters();

        // Build type signature using GenericType::toString() to preserve generics
        std::vector<std::string> typeNames;
        typeNames.reserve(genericParams.size());
        for (const auto& [paramName, genericType] : genericParams) {
            typeNames.push_back(genericType->toString());
        }

        std::string typeSignature = runtimeTypes::klass::SignatureUtils::generateTypeSignatureFromNames(typeNames);
        std::string resolvedName = runtimeTypes::klass::SignatureUtils::buildQualifiedName(baseClassName, methodName, typeSignature);
        return resolvedName + "$static";
    }

    std::string OverloadResolutionHelper::resolveFunctionOverload(
        const std::string& functionName,
        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
        const ast::SourceLocation& location,
        bool hasGenericTypeArgs,
        size_t genericTypeArgCount)
    {
        // Get function registry
        auto funcRegistry = ctx.environment->getFunctionRegistry();
        if (!funcRegistry)
        {
            // No registry - return plain name
            return functionName;
        }

        // Get all function overloads
        auto overloads = funcRegistry->getAllFunctionOverloads(functionName);

        if (overloads.empty())
        {
            // No functions found - return plain name
            return functionName;
        }

        // Filter overloads based on generic type arguments (Java/C# style)
        std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>> filteredOverloads;
        if (hasGenericTypeArgs)
        {
            // Only consider generic overloads with matching number of type parameters
            for (const auto& overload : overloads)
            {
                size_t genericParamCount = overload->getGenericTypeParameters().size();
                if (genericParamCount == genericTypeArgCount)
                {
                    filteredOverloads.push_back(overload);
                }
            }
        }
        else
        {
            // NO explicit type arguments provided
            // Java/C# rule: Prefer non-generic overloads over generic ones
            std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>> nonGenericOverloads;
            std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>> genericOverloads;

            for (const auto& overload : overloads)
            {
                size_t genericParamCount = overload->getGenericTypeParameters().size();
                const auto& params = overload->getParameters();

                // Check parameter types to see if they contain unresolved generics
                bool hasUnresolvedGenerics = (genericParamCount > 0);

                for (const auto& [paramName, paramType] : params) {
                    std::string typeStr;
                    if (paramType.className.has_value()) {
                        typeStr = paramType.className.value();
                    } else {
                        // Primitive type - use basicType
                        typeStr = runtimeTypes::klass::SignatureUtils::getTypeName(paramType);
                    }

                    // Check if type contains unresolved generic parameters (like T, K, V)
                    auto& registry = ::types::getGlobalTypeRegistry();

                    // Simple check: if type string is a single capital letter and not in registry
                    if (typeStr.length() == 1 && std::isupper(typeStr[0]) && !registry.hasType(typeStr)) {
                        hasUnresolvedGenerics = true;
                    }
                    // Check for parameterized types with generics: Box<T>, List<K>, etc.
                    else if (typeStr.find('<') != std::string::npos) {
                        size_t start = typeStr.find('<');
                        size_t end = typeStr.find('>');
                        if (end != std::string::npos && end > start) {
                            std::string typeArg = typeStr.substr(start + 1, end - start - 1);
                            // Trim spaces
                            typeArg.erase(0, typeArg.find_first_not_of(" \t"));
                            typeArg.erase(typeArg.find_last_not_of(" \t") + 1);

                            // Check if type argument is unresolved (single letter not in registry)
                            if (typeArg.length() == 1 && std::isupper(typeArg[0]) && !registry.hasType(typeArg)) {
                                hasUnresolvedGenerics = true;
                            }
                        }
                    }
                }

                if (!hasUnresolvedGenerics)
                {
                    // Non-generic function (all types are concrete)
                    nonGenericOverloads.push_back(overload);
                }
                else
                {
                    // Generic function (contains unresolved type parameters)
                    genericOverloads.push_back(overload);
                }
            }

            // If we have non-generic overloads, use only those
            // Otherwise, fall back to generic overloads
            if (!nonGenericOverloads.empty())
            {
                filteredOverloads = nonGenericOverloads;
            }
            else
            {
                filteredOverloads = genericOverloads;
            }
        }

        if (filteredOverloads.empty())
        {
            // No matching overloads after filtering
            return functionName;
        }

        if (filteredOverloads.size() == 1)
        {
            // Only one overload after filtering - return mangled name
            const auto& func = filteredOverloads[0];
            std::string mangledName = buildMangledFunctionName(functionName, func->getParameters());
            return mangledName;
        }

        // Multiple overloads - need resolution
        std::vector<value::ParameterType> argTypes = inferArgumentTypes(arguments);

        // Use OverloadResolver to find best match (using filtered overloads)
        auto result = OverloadResolver::resolveFunctionOverload(filteredOverloads, argTypes, location);

        if (result.isAmbiguous)
        {
            // Build argument type strings
            std::vector<std::string> argTypeStrings;
            for (const auto& argType : argTypes)
            {
                argTypeStrings.push_back(runtimeTypes::klass::SignatureUtils::getTypeName(argType));
            }

            // Build candidate signature strings
            std::vector<std::string> candidateSignatures;
            for (const auto& candidate : result.ambiguousCandidates)
            {
                candidateSignatures.push_back(
                    runtimeTypes::klass::SignatureUtils::generateTypeSignature(candidate->getParameters())
                );
            }

            throw errors::AmbiguousCallException(
                functionName,
                candidateSignatures,
                argTypeStrings,
                location
            );
        }

        if (!result.hasViableCandidates || !result.selectedOverload)
        {
            // Build argument type strings
            std::vector<std::string> argTypeStrings;
            for (const auto& argType : argTypes)
            {
                argTypeStrings.push_back(runtimeTypes::klass::SignatureUtils::getTypeName(argType));
            }

            // Build available signature strings
            std::vector<std::string> availableSignatures;
            for (const auto& overload : filteredOverloads)
            {
                availableSignatures.push_back(
                    runtimeTypes::klass::SignatureUtils::generateTypeSignature(overload->getParameters())
                );
            }

            throw errors::NoMatchingOverloadException(
                functionName,
                availableSignatures,
                argTypeStrings,
                location
            );
        }

        // Successfully resolved
        std::string resolvedName = buildMangledFunctionName(functionName, result.selectedOverload->getParameters());
        return resolvedName;
    }
}
