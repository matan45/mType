#include "OverloadResolutionHelper.hpp"
#include <cstddef>
#include "OverloadResolver.hpp"
#include "../../../environment/registry/SignatureUtils.hpp"
#include "../../../errors/NoMatchingOverloadException.hpp"
#include "../../../errors/AmbiguousCallException.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../environment/registry/TypeCatalog.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include <unordered_map>

namespace vm::compiler::overload
{
    // Substitutes only complete identifier tokens — guards against substring
    // matches like 'T' inside "Type" or "transactionId".
    std::string OverloadResolutionHelper::substituteTypeParameters(
        const std::string& typeStr,
        const std::unordered_map<std::string, std::string>& substitutions)
    {
        std::string result = typeStr;

        for (const auto& [param, replacement] : substitutions)
        {
            size_t pos = 0;
            while ((pos = result.find(param, pos)) != std::string::npos)
            {
                bool isCompleteToken = true;

                if (pos > 0)
                {
                    char prevChar = result[pos - 1];
                    if (std::isalnum(prevChar) || prevChar == '_')
                    {
                        isCompleteToken = false;
                    }
                }

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
            // A literal `null` must be reported as NULL_TYPE, not OBJECT-untyped.
            // Type inference can collapse `null` to OBJECT (no className), and the
            // resolver's OBJECT-vs-OBJECT path then declares every typed-OBJECT
            // overload INCOMPATIBLE — masking the real "null matches both" case
            // as "no matching overload (object)". Forcing NULL_TYPE here lets the
            // existing null → OBJECT EXACT_MATCH branch run, which keeps every
            // viable overload alive so ambiguity detection can fire.
            if (dynamic_cast<ast::NullNode*>(arg.get()))
            {
                argTypes.emplace_back(value::ParameterType(value::ValueType::NULL_TYPE));
                continue;
            }

            value::ValueType basicType = ctx.typeInference.inferExpressionType(arg.get());

            if (basicType == value::ValueType::OBJECT)
            {
                std::string className = ctx.typeInference.inferExpressionClassName(arg.get());

                if (!className.empty())
                {
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
                    argTypes.emplace_back(value::ParameterType(basicType));
                }
            }
            else
            {
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
        return runtimeTypes::klass::SignatureUtils::buildQualifiedName(className, methodName, parameters);
    }

    std::string OverloadResolutionHelper::buildMangledFunctionName(
        const std::string& functionName,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters)
    {
        std::string signature = runtimeTypes::klass::SignatureUtils::generateTypeSignature(parameters);
        if (signature.empty())
        {
            return functionName;
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
        std::string baseClassName = ::types::TypeConversionUtils::stripNullable(className);
        size_t anglePos = baseClassName.find('<');
        if (anglePos != std::string::npos)
        {
            baseClassName = baseClassName.substr(0, anglePos);
        }

        auto classDef = ctx.env->findClass(baseClassName);
        if (!classDef)
        {
            // Runtime handles class-not-found error.
            return methodName;
        }

        auto overloads = classDef->getAllInstanceMethodOverloadsInHierarchy(methodName);

        if (overloads.empty())
        {
            // Runtime handles method-not-found error.
            return methodName;
        }

        std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>> filteredOverloads;

        if (hasGenericTypeArgs)
        {
            // Explicit type arguments: method<Type>() — keep only matching arity.
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
            // Java/C# rule: prefer non-generic overloads over generic ones.
            std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>> nonGenericOverloads;
            std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>> genericOverloads;

            for (const auto& overload : overloads)
            {
                const auto& uParams = overload->getUnifiedParameters();

                bool hasUnresolvedGenerics = false;
                for (const auto& [paramName, uType] : uParams) {
                    std::string typeStr = uType ? uType->toString() : "void";

                    auto& catalog = *ctx.env->getTypeCatalog();

                    if (typeStr.length() == 1 && std::isupper(typeStr[0]) && !catalog.hasType(typeStr)) {
                        hasUnresolvedGenerics = true;
                    }
                    else if (typeStr.find('<') != std::string::npos) {
                        size_t start = typeStr.find('<');
                        size_t end = typeStr.find('>');
                        if (end != std::string::npos && end > start) {
                            std::string typeArg = typeStr.substr(start + 1, end - start - 1);
                            typeArg.erase(0, typeArg.find_first_not_of(" \t"));
                            typeArg.erase(typeArg.find_last_not_of(" \t") + 1);

                            if (typeArg.length() == 1 && std::isupper(typeArg[0]) && !catalog.hasType(typeArg)) {
                                hasUnresolvedGenerics = true;
                            }
                        }
                    }
                }

                if (!hasUnresolvedGenerics)
                {
                    nonGenericOverloads.push_back(overload);
                }
                else
                {
                    genericOverloads.push_back(overload);
                }
            }

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
            return methodName;
        }

        if (filteredOverloads.size() == 1)
        {
            // Single survivor: emit mangled name. UnifiedType preserves
            // generic signatures (e.g., UnaryFunction<T>). genericParameters
            // skips 'this' already.
            const auto& method = filteredOverloads[0];
            const auto& uParams = method->getUnifiedParameters();

            std::vector<std::string> typeNames;
            typeNames.reserve(uParams.size());
            for (const auto& [paramName, uType] : uParams) {
                typeNames.push_back(uType ? uType->toString() : "void");
            }

            std::string typeSignature = runtimeTypes::klass::SignatureUtils::generateTypeSignatureFromNames(typeNames);
            std::string mangledName = runtimeTypes::klass::SignatureUtils::buildQualifiedName(baseClassName, methodName, typeSignature);
            return mangledName;
        }

        std::vector<value::ParameterType> argTypes = inferArgumentTypes(arguments);

        auto result = OverloadResolver::resolveMethodOverload(filteredOverloads, argTypes, location, *ctx.env->getTypeCatalog());

        if (result.isAmbiguous)
        {
            std::vector<std::string> argTypeStrings;
            for (const auto& argType : argTypes)
            {
                argTypeStrings.push_back(runtimeTypes::klass::SignatureUtils::getTypeName(argType));
            }

            // Skip 'this' parameter from error messages on instance methods.
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
            std::vector<std::string> argTypeStrings;
            for (const auto& argType : argTypes)
            {
                argTypeStrings.push_back(runtimeTypes::klass::SignatureUtils::getTypeName(argType));
            }

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

        const auto& uParams = result.selectedOverload->getUnifiedParameters();

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
}
