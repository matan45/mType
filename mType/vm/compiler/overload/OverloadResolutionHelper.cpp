#include "OverloadResolutionHelper.hpp"
#include "OverloadResolver.hpp"
#include "../../../runtimeTypes/klass/SignatureUtils.hpp"
#include "../../../errors/NoMatchingOverloadException.hpp"
#include "../../../errors/AmbiguousCallException.hpp"
#include "../../../types/TypeConversionUtils.hpp"

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

        // Filter overloads based on generic type arguments (if provided)
        std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>> filteredOverloads;
        if (hasGenericTypeArgs)
        {
            // Only consider generic overloads with matching number of generic type parameters
            // Note: Methods store generic parameters in the ParameterType with className as generic names
            for (const auto& overload : overloads)
            {
                // Count how many parameters are generic (have className but not a real class)
                const auto& params = overload->getParametersWithTypes();
                size_t genericParamCount = 0;
                for (const auto& [paramName, paramType] : params)
                {
                    if (paramType.basicType == value::ValueType::OBJECT && paramType.className.has_value())
                    {
                        // Check if className is a generic parameter (single letter like T, K, V)
                        // Simple heuristic: if it's not in the class registry, it's likely generic
                        auto classRegistry = ctx.environment->getClassRegistry();
                        if (!classRegistry || !classRegistry->findClass(paramType.className.value()))
                        {
                            genericParamCount++;
                        }
                    }
                }

                if (genericParamCount == genericTypeArgCount)
                {
                    filteredOverloads.push_back(overload);
                }
            }
        }
        else
        {
            // No generic type arguments - use all overloads
            filteredOverloads = overloads;
        }

        if (filteredOverloads.empty())
        {
            // No matching overloads after filtering
            return methodName;
        }

        if (filteredOverloads.size() == 1)
        {
            // Only one overload after filtering - return mangled name
            const auto& method = filteredOverloads[0];
            // Skip 'this' parameter when building mangled name for instance methods
            auto params = method->getParametersWithTypes();
            if (!method->isStatic() && !params.empty()) {
                params = std::vector<std::pair<std::string, value::ParameterType>>(params.begin() + 1, params.end());
            }
            std::string mangledName = buildMangledMethodName(baseClassName, methodName, params);
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
        // Skip 'this' parameter when building mangled name for instance methods
        auto params = result.selectedOverload->getParametersWithTypes();
        if (!result.selectedOverload->isStatic() && !params.empty()) {
            params = std::vector<std::pair<std::string, value::ParameterType>>(params.begin() + 1, params.end());
        }
        std::string resolvedName = buildMangledMethodName(baseClassName, methodName, params);
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
            const auto& method = overloads[0];
            std::string mangledName = buildMangledMethodName(baseClassName, methodName, method->getParametersWithTypes());
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
        std::string mangledName = buildMangledMethodName(baseClassName, methodName, result.selectedOverload->getParametersWithTypes());
        return mangledName + "$static";
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

        // Filter overloads based on generic type arguments
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
            // No generic type arguments - prefer non-generic overloads
            // But if only generic overloads exist, include them for type inference
            for (const auto& overload : overloads)
            {
                filteredOverloads.push_back(overload);
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
