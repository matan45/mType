#include "OverloadResolutionHelper.hpp"
#include <cstddef>
#include "OverloadResolver.hpp"
#include "../../../environment/registry/SignatureUtils.hpp"
#include "../../../errors/NoMatchingOverloadException.hpp"
#include "../../../errors/AmbiguousCallException.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../environment/registry/TypeCatalog.hpp"
#include <iostream>
#include <unordered_map>
#include <algorithm>

namespace vm::compiler::overload
{
    /**
     * Helper function to substitute generic type parameters in a type string
     * For example: substituteTypeParameters("Wrapper<T>", {{"T", "Int"}}) => "Wrapper<Int>"
     */
    static std::string substituteTypeParameters(
        const std::string& typeStr,
        const std::unordered_map<std::string, std::string>& substitutions)
    {
        std::string result = typeStr;

        // Simple substitution: replace each type parameter with its corresponding type
        // We need to be careful to only substitute complete tokens, not substrings
        for (const auto& [param, replacement] : substitutions)
        {
            // Look for the parameter as a complete token
            size_t pos = 0;
            while ((pos = result.find(param, pos)) != std::string::npos)
            {
                // Check if this is a complete token (not part of a larger identifier)
                bool isCompleteToken = true;

                // Check character before (if exists)
                if (pos > 0)
                {
                    char prevChar = result[pos - 1];
                    if (std::isalnum(prevChar) || prevChar == '_')
                    {
                        isCompleteToken = false;
                    }
                }

                // Check character after (if exists)
                if (isCompleteToken && pos + param.length() < result.length())
                {
                    char nextChar = result[pos + param.length()];
                    if (std::isalnum(nextChar) || nextChar == '_')
                    {
                        isCompleteToken = false;
                    }
                }

                if (isCompleteToken)
                {
                    result.replace(pos, param.length(), replacement);
                    pos += replacement.length();
                }
                else
                {
                    pos += param.length();
                }
            }
        }

        return result;
    }

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
                    auto interfaceDef = ctx.env->findInterface(className);
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
        // Extract base class name (without nullable suffix or generic parameters)
        std::string baseClassName = ::types::TypeConversionUtils::stripNullable(className);
        size_t anglePos = baseClassName.find('<');
        if (anglePos != std::string::npos)
        {
            baseClassName = baseClassName.substr(0, anglePos);
        }

        // Get class definition
        auto classDef = ctx.env->findClass(baseClassName);
        if (!classDef)
        {
            // Class not found - return plain method name (runtime will handle error)
            return methodName;
        }

        // Get all instance method overloads including inherited methods
        auto overloads = classDef->getAllInstanceMethodOverloadsInHierarchy(methodName);

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
                const auto& uParams = overload->getUnifiedParameters();

                // Check parameter types using UnifiedType to see if they contain unresolved generics
                bool hasUnresolvedGenerics = false;
                for (const auto& [paramName, uType] : uParams) {
                    std::string typeStr = uType ? uType->toString() : "void";

                    // Check if this type contains unresolved generic parameters (like T, K, V)
                    // Look for patterns like "T", "Box<T>", "List<K>", etc.
                    auto& catalog = *ctx.env->getTypeCatalog();

                    // Simple check: if type string is a single capital letter and not in registry
                    if (typeStr.length() == 1 && std::isupper(typeStr[0]) && !catalog.hasType(typeStr)) {
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
                            if (typeArg.length() == 1 && std::isupper(typeArg[0]) && !catalog.hasType(typeArg)) {
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
            const auto& uParams = method->getUnifiedParameters();

            // Build type signature using UnifiedType::toString() to preserve generics
            std::vector<std::string> typeNames;
            typeNames.reserve(uParams.size());
            for (const auto& [paramName, uType] : uParams) {
                typeNames.push_back(uType ? uType->toString() : "void");
            }

            std::string typeSignature = runtimeTypes::klass::SignatureUtils::generateTypeSignatureFromNames(typeNames);
            std::string mangledName = runtimeTypes::klass::SignatureUtils::buildQualifiedName(baseClassName, methodName, typeSignature);
            return mangledName;
        }

        // Multiple overloads - need resolution
        std::vector<value::ParameterType> argTypes = inferArgumentTypes(arguments);

        // Use OverloadResolver to find best match (using filtered overloads)
        auto result = OverloadResolver::resolveMethodOverload(filteredOverloads, argTypes, location, *ctx.env->getTypeCatalog());

        if (result.isAmbiguous)
        {
            // Build argument type strings
            std::vector<std::string> argTypeStrings;
            for (const auto& argType : argTypes)
            {
                argTypeStrings.push_back(runtimeTypes::klass::SignatureUtils::getTypeName(argType));
            }

            // Build candidate signature strings
            // For instance methods, skip the 'this' parameter (first parameter) in error messages
            std::vector<std::string> candidateSignatures;
            for (const auto& candidate : result.ambiguousCandidates)
            {
                auto params = candidate->getParametersWithTypes();
                if (!candidate->isStatic() && !params.empty() && params[0].first == "this")
                {
                    params.erase(params.begin());
                }
                candidateSignatures.push_back(
                    runtimeTypes::klass::SignatureUtils::generateTypeSignature(params)
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
            // For instance methods, skip the 'this' parameter (first parameter) in error messages
            std::vector<std::string> availableSignatures;
            for (const auto& overload : filteredOverloads)
            {
                auto params = overload->getParametersWithTypes();
                if (!overload->isStatic() && !params.empty() && params[0].first == "this")
                {
                    params.erase(params.begin());
                }
                availableSignatures.push_back(
                    runtimeTypes::klass::SignatureUtils::generateTypeSignature(params)
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
        // Use unifiedParameters to preserve full type signatures (e.g., UnaryFunction<T>)
        // Skip 'this' parameter for instance methods (unifiedParameters doesn't include it)
        const auto& uParams = result.selectedOverload->getUnifiedParameters();

        // Build type signature using UnifiedType::toString() to preserve generics
        std::vector<std::string> typeNames;
        typeNames.reserve(uParams.size());
        for (const auto& [paramName, uType] : uParams) {
            std::string typeName = uType ? uType->toString() : "void";
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
        const ast::SourceLocation& location,
        bool hasGenericTypeArgs,
        const std::vector<std::string>& genericTypeArgs)
    {
        // Extract base class name (without nullable suffix or generic parameters)
        std::string baseClassName = ::types::TypeConversionUtils::stripNullable(className);
        size_t anglePos = baseClassName.find('<');
        if (anglePos != std::string::npos)
        {
            baseClassName = baseClassName.substr(0, anglePos);
        }

        // Get class definition
        auto classDef = ctx.env->findClass(baseClassName);
        if (!classDef)
        {
            // Class not found - return qualified plain name
            return baseClassName + "::" + methodName;
        }

        // Get all static method overloads including inherited methods
        auto overloads = classDef->getAllStaticMethodOverloadsInHierarchy(methodName);
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
            const auto& uParams = method->getUnifiedParameters();

            // Build type signature using UnifiedType::toString() to preserve generics
            std::vector<std::string> typeNames;
            typeNames.reserve(uParams.size());
            for (const auto& [paramName, uType] : uParams) {
                typeNames.push_back(uType ? uType->toString() : "void");
            }

            std::string typeSignature = runtimeTypes::klass::SignatureUtils::generateTypeSignatureFromNames(typeNames);
            std::string mangledName = runtimeTypes::klass::SignatureUtils::buildQualifiedName(baseClassName, methodName, typeSignature);
            return mangledName + "$static";
        }

        std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>> filteredOverloads;

        // MYT-224: when the call provides explicit generic type arguments, substitute
        // them into each candidate's declared parameter types and keep only those that
        // unify with the actual argument types. Without this step, two competing generic
        // overloads with parameterized parameters (e.g. UnaryFn<T,R> vs Predicate<T>)
        // can't be disambiguated by the structural compare in OverloadResolver.
        if (hasGenericTypeArgs)
        {
            std::vector<value::ParameterType> argTypes = inferArgumentTypes(arguments);

            std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>> substitutionMatches;
            for (const auto& overload : overloads)
            {
                const auto& typeParams = overload->getGenericTypeParameters();
                if (typeParams.size() != genericTypeArgs.size())
                {
                    continue;
                }

                std::unordered_map<std::string, std::string> substitutions;
                for (size_t i = 0; i < typeParams.size(); ++i)
                {
                    substitutions[typeParams[i].name] = genericTypeArgs[i];
                }

                // Static methods carry no implicit 'this' entry in getParametersWithTypes().
                auto params = overload->getParametersWithTypes();
                if (params.size() != argTypes.size())
                {
                    continue;
                }

                bool isCompatible = true;
                for (size_t i = 0; i < params.size(); ++i)
                {
                    const auto& [paramName, paramType] = params[i];
                    const auto& argType = argTypes[i];

                    std::string paramTypeStr = paramType.className.has_value()
                        ? paramType.className.value()
                        : runtimeTypes::klass::SignatureUtils::getTypeName(paramType);

                    std::string substitutedTypeStr = substituteTypeParameters(paramTypeStr, substitutions);

                    std::string argTypeStr = argType.className.has_value()
                        ? argType.className.value()
                        : runtimeTypes::klass::SignatureUtils::getTypeName(argType);

                    bool typesMatch = (substitutedTypeStr == argTypeStr);
                    if (!typesMatch)
                    {
                        // Primitive case-insensitive fallback (e.g. "Int" vs "int").
                        std::string lowerSubstituted = substitutedTypeStr;
                        std::string lowerArg = argTypeStr;
                        std::transform(lowerSubstituted.begin(), lowerSubstituted.end(), lowerSubstituted.begin(), ::tolower);
                        std::transform(lowerArg.begin(), lowerArg.end(), lowerArg.begin(), ::tolower);
                        typesMatch = (lowerSubstituted == lowerArg);
                    }

                    if (!typesMatch)
                    {
                        isCompatible = false;
                        break;
                    }
                }

                if (isCompatible)
                {
                    substitutionMatches.push_back(overload);
                }
            }

            // Exactly one survivor — emit the resolved name directly. OverloadResolver
            // can't be relied on here because the candidate's parameter strings still
            // contain unsubstituted T/R that don't equal the concrete argument types.
            if (substitutionMatches.size() == 1)
            {
                const auto& method = substitutionMatches[0];
                const auto& uParams = method->getUnifiedParameters();

                std::vector<std::string> typeNames;
                typeNames.reserve(uParams.size());
                for (const auto& [paramName, uType] : uParams) {
                    typeNames.push_back(uType ? uType->toString() : "void");
                }

                std::string typeSignature = runtimeTypes::klass::SignatureUtils::generateTypeSignatureFromNames(typeNames);
                std::string mangledName = runtimeTypes::klass::SignatureUtils::buildQualifiedName(baseClassName, methodName, typeSignature);
                return mangledName + "$static";
            }

            // 0 or 2+ — narrow only when narrowing helps; otherwise keep the full set
            // so the existing error path still lists every available overload.
            filteredOverloads = substitutionMatches.empty() ? overloads : substitutionMatches;
        }
        else
        {
            filteredOverloads = overloads;
        }

        // Multiple overloads - need resolution
        std::vector<value::ParameterType> argTypes = inferArgumentTypes(arguments);

        // Use OverloadResolver to find best match
        auto result = OverloadResolver::resolveMethodOverload(filteredOverloads, argTypes, location, *ctx.env->getTypeCatalog());

        if (result.isAmbiguous)
        {
            // Build argument type strings
            std::vector<std::string> argTypeStrings;
            for (const auto& argType : argTypes)
            {
                argTypeStrings.push_back(runtimeTypes::klass::SignatureUtils::getTypeName(argType));
            }

            // Build candidate signature strings
            // For instance methods, skip the 'this' parameter (first parameter) in error messages
            std::vector<std::string> candidateSignatures;
            for (const auto& candidate : result.ambiguousCandidates)
            {
                auto params = candidate->getParametersWithTypes();
                if (!candidate->isStatic() && !params.empty() && params[0].first == "this")
                {
                    params.erase(params.begin());
                }
                candidateSignatures.push_back(
                    runtimeTypes::klass::SignatureUtils::generateTypeSignature(params)
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
            // For instance methods, skip the 'this' parameter (first parameter) in error messages
            std::vector<std::string> availableSignatures;
            for (const auto& overload : filteredOverloads)
            {
                auto params = overload->getParametersWithTypes();
                if (!overload->isStatic() && !params.empty() && params[0].first == "this")
                {
                    params.erase(params.begin());
                }
                availableSignatures.push_back(
                    runtimeTypes::klass::SignatureUtils::generateTypeSignature(params)
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
        // Use unifiedParameters to preserve full type signatures (e.g., UnaryFunction<T>)
        const auto& uParams = result.selectedOverload->getUnifiedParameters();

        // Build type signature using UnifiedType::toString() to preserve generics
        std::vector<std::string> typeNames;
        typeNames.reserve(uParams.size());
        for (const auto& [paramName, uType] : uParams) {
            typeNames.push_back(uType ? uType->toString() : "void");
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
        const std::vector<std::string>& genericTypeArgs)
    {
        size_t genericTypeArgCount = genericTypeArgs.size();
        // Get function registry
        auto funcRegistry = ctx.env->getFunctionRegistry();
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
        std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>> nonGenericOverloads;
        std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>> genericOverloads;

        if (hasGenericTypeArgs)
        {
            // When explicit type arguments are provided, we need to:
            // 1. Filter by generic parameter count
            // 2. Substitute type arguments into parameter types
            // 3. Check if substituted types are compatible with actual argument types

            // Infer actual argument types for compatibility checking
            std::vector<value::ParameterType> argTypes = inferArgumentTypes(arguments);

            for (const auto& overload : overloads)
            {
                size_t genericParamCount = overload->getGenericTypeParameters().size();
                if (genericParamCount != genericTypeArgCount)
                {
                    continue; // Wrong number of type parameters
                }

                // Build substitution map: T -> Int, K -> String, etc.
                std::unordered_map<std::string, std::string> substitutions;
                const auto& typeParams = overload->getGenericTypeParameters();
                for (size_t i = 0; i < typeParams.size(); ++i)
                {
                    substitutions[typeParams[i].name] = genericTypeArgs[i];
                }

                // Check if substituted parameter types match argument types
                const auto& params = overload->getParameters();
                if (params.size() != argTypes.size())
                {
                    continue; // Wrong number of parameters
                }

                bool isCompatible = true;
                for (size_t i = 0; i < params.size(); ++i)
                {
                    const auto& [paramName, paramType] = params[i];
                    const auto& argType = argTypes[i];

                    // Get parameter type string
                    std::string paramTypeStr;
                    if (paramType.className.has_value())
                    {
                        paramTypeStr = paramType.className.value();
                    }
                    else
                    {
                        paramTypeStr = runtimeTypes::klass::SignatureUtils::getTypeName(paramType);
                    }

                    // Substitute type parameters
                    std::string substitutedTypeStr = substituteTypeParameters(paramTypeStr, substitutions);

                    // Get argument type string
                    std::string argTypeStr;
                    if (argType.className.has_value())
                    {
                        argTypeStr = argType.className.value();
                    }
                    else
                    {
                        argTypeStr = runtimeTypes::klass::SignatureUtils::getTypeName(argType);
                    }

                    // Check if types match
                    // Note: Need to handle case differences between primitive types (string) and classes (String)
                    bool typesMatch = (substitutedTypeStr == argTypeStr);

                    // If exact match fails, try case-insensitive comparison for primitive types
                    if (!typesMatch)
                    {
                        // Convert both to lowercase for comparison
                        std::string lowerSubstituted = substitutedTypeStr;
                        std::string lowerArg = argTypeStr;
                        std::transform(lowerSubstituted.begin(), lowerSubstituted.end(), lowerSubstituted.begin(), ::tolower);
                        std::transform(lowerArg.begin(), lowerArg.end(), lowerArg.begin(), ::tolower);
                        typesMatch = (lowerSubstituted == lowerArg);
                    }

                    if (!typesMatch)
                    {
                        isCompatible = false;
                        break;
                    }
                }

                if (isCompatible)
                {
                    filteredOverloads.push_back(overload);
                }
            }
        }
        else
        {
            // NO explicit type arguments provided
            // Java/C# rule: Prefer non-generic overloads over generic ones

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
                    auto& catalog = *ctx.env->getTypeCatalog();

                    // Simple check: if type string is a single capital letter and not in registry
                    if (typeStr.length() == 1 && std::isupper(typeStr[0]) && !catalog.hasType(typeStr)) {
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
                            if (typeArg.length() == 1 && std::isupper(typeArg[0]) && !catalog.hasType(typeArg)) {
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

            // Java/C# rule: Prefer non-generic overloads over generic ones
            // Try non-generic first, but fall back to generic if no viable matches
            filteredOverloads = nonGenericOverloads;
        }

        if (filteredOverloads.empty())
        {
            // No matching overloads after filtering
            return functionName;
        }

        // Infer argument types for overload resolution
        std::vector<value::ParameterType> argTypes = inferArgumentTypes(arguments);

        if (filteredOverloads.size() == 1 && argTypes.size() == filteredOverloads[0]->getParameters().size())
        {
            // Only one overload after filtering and parameter count matches - return mangled name
            const auto& func = filteredOverloads[0];
            std::string mangledName = buildMangledFunctionName(functionName, func->getParameters());
            return mangledName;
        }

        // Try to resolve with non-generic overloads first
        auto result = OverloadResolver::resolveFunctionOverload(filteredOverloads, argTypes, location, *ctx.env->getTypeCatalog());

        // If no viable candidates from non-generic overloads, try generic overloads
        if ((!result.hasViableCandidates || !result.selectedOverload) && !hasGenericTypeArgs && !genericOverloads.empty())
        {
            filteredOverloads = genericOverloads;
            result = OverloadResolver::resolveFunctionOverload(filteredOverloads, argTypes, location, *ctx.env->getTypeCatalog());
        }

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

