#include "OverloadResolutionHelper.hpp"
#include <cstddef>
#include "OverloadResolver.hpp"
#include "../../../environment/registry/SignatureUtils.hpp"
#include "../../../errors/NoMatchingOverloadException.hpp"
#include "../../../errors/AmbiguousCallException.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../environment/registry/TypeCatalog.hpp"
#include <unordered_map>
#include <algorithm>

namespace vm::compiler::overload
{
    std::string OverloadResolutionHelper::resolveStaticMethodOverload(
        const std::string& className,
        const std::string& methodName,
        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
        const ast::SourceLocation& location,
        bool hasGenericTypeArgs,
        const std::vector<std::string>& genericTypeArgs)
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
            return baseClassName + "::" + methodName;
        }

        auto overloads = classDef->getAllStaticMethodOverloadsInHierarchy(methodName);
        if (overloads.empty())
        {
            return baseClassName + "::" + methodName;
        }

        if (overloads.size() == 1)
        {
            // Single overload: emit mangled name with $static suffix.
            // UnifiedType preserves generic signatures (e.g., UnaryFunction<T>).
            const auto& method = overloads[0];
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

        std::vector<std::shared_ptr<runtimeTypes::klass::MethodDefinition>> filteredOverloads;

        // MYT-224: when the call provides explicit generic type arguments,
        // substitute them into each candidate's declared parameter types and
        // keep only those that unify with the actual argument types. Without
        // this step, two competing generic overloads with parameterized
        // parameters (e.g. UnaryFn<T,R> vs Predicate<T>) can't be
        // disambiguated by the structural compare in OverloadResolver.
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

            // Exactly one survivor — emit the resolved name directly.
            // OverloadResolver can't be relied on here because the candidate's
            // parameter strings still contain unsubstituted T/R that don't
            // equal the concrete argument types.
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

            // 0 or 2+ — keep the full set so the existing error path still
            // lists every available overload.
            filteredOverloads = substitutionMatches.empty() ? overloads : substitutionMatches;
        }
        else
        {
            filteredOverloads = overloads;
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
            typeNames.push_back(uType ? uType->toString() : "void");
        }

        std::string typeSignature = runtimeTypes::klass::SignatureUtils::generateTypeSignatureFromNames(typeNames);
        std::string resolvedName = runtimeTypes::klass::SignatureUtils::buildQualifiedName(baseClassName, methodName, typeSignature);
        return resolvedName + "$static";
    }
}
