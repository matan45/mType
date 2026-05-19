#include "OverloadResolutionHelper.hpp"
#include <cstddef>
#include "OverloadResolver.hpp"
#include "../../../environment/registry/SignatureUtils.hpp"
#include "../../../errors/NoMatchingOverloadException.hpp"
#include "../../../errors/AmbiguousCallException.hpp"
#include "../../../environment/registry/TypeCatalog.hpp"
#include <unordered_map>
#include <algorithm>

namespace vm::compiler::overload
{
    std::string OverloadResolutionHelper::resolveFunctionOverload(
        const std::string& functionName,
        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
        const ast::SourceLocation& location,
        bool hasGenericTypeArgs,
        const std::vector<std::string>& genericTypeArgs)
    {
        size_t genericTypeArgCount = genericTypeArgs.size();
        auto funcRegistry = ctx.env->getFunctionRegistry();
        if (!funcRegistry)
        {
            return functionName;
        }

        auto overloads = funcRegistry->getAllFunctionOverloads(functionName);

        if (overloads.empty())
        {
            return functionName;
        }

        std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>> filteredOverloads;
        std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>> nonGenericOverloads;
        std::vector<std::shared_ptr<runtimeTypes::global::FunctionDefinition>> genericOverloads;

        if (hasGenericTypeArgs)
        {
            // With explicit type arguments: filter by generic arity, substitute
            // type arguments into parameter types, then check compatibility
            // with actual argument types.
            std::vector<value::ParameterType> argTypes = inferArgumentTypes(arguments);

            for (const auto& overload : overloads)
            {
                size_t genericParamCount = overload->getGenericTypeParameters().size();
                if (genericParamCount != genericTypeArgCount)
                {
                    continue;
                }

                std::unordered_map<std::string, std::string> substitutions;
                const auto& typeParams = overload->getGenericTypeParameters();
                for (size_t i = 0; i < typeParams.size(); ++i)
                {
                    substitutions[typeParams[i].name] = genericTypeArgs[i];
                }

                const auto& params = overload->getParameters();
                if (params.size() != argTypes.size())
                {
                    continue;
                }

                bool isCompatible = true;
                for (size_t i = 0; i < params.size(); ++i)
                {
                    const auto& [paramName, paramType] = params[i];
                    const auto& argType = argTypes[i];

                    std::string paramTypeStr;
                    if (paramType.className.has_value())
                    {
                        paramTypeStr = paramType.className.value();
                    }
                    else
                    {
                        paramTypeStr = runtimeTypes::klass::SignatureUtils::getTypeName(paramType);
                    }

                    std::string substitutedTypeStr = substituteTypeParameters(paramTypeStr, substitutions);

                    std::string argTypeStr;
                    if (argType.className.has_value())
                    {
                        argTypeStr = argType.className.value();
                    }
                    else
                    {
                        argTypeStr = runtimeTypes::klass::SignatureUtils::getTypeName(argType);
                    }

                    bool typesMatch = (substitutedTypeStr == argTypeStr);

                    // Primitive case-insensitive fallback (e.g. "string" vs "String").
                    if (!typesMatch)
                    {
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
            // Java/C# rule: prefer non-generic overloads over generic ones.
            for (const auto& overload : overloads)
            {
                size_t genericParamCount = overload->getGenericTypeParameters().size();
                const auto& params = overload->getParameters();

                bool hasUnresolvedGenerics = (genericParamCount > 0);

                for (const auto& [paramName, paramType] : params) {
                    std::string typeStr;
                    if (paramType.className.has_value()) {
                        typeStr = paramType.className.value();
                    } else {
                        typeStr = runtimeTypes::klass::SignatureUtils::getTypeName(paramType);
                    }

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

            // Try non-generic first; fall back to generic if no viable matches.
            filteredOverloads = nonGenericOverloads;
        }

        if (filteredOverloads.empty())
        {
            return functionName;
        }

        std::vector<value::ParameterType> argTypes = inferArgumentTypes(arguments);

        if (filteredOverloads.size() == 1 && argTypes.size() == filteredOverloads[0]->getParameters().size())
        {
            const auto& func = filteredOverloads[0];
            std::string mangledName = buildMangledFunctionName(functionName, func->getParameters());
            return mangledName;
        }

        auto result = OverloadResolver::resolveFunctionOverload(filteredOverloads, argTypes, location, *ctx.env->getTypeCatalog());

        // Fall back to generic overloads if non-generic resolution found nothing.
        if ((!result.hasViableCandidates || !result.selectedOverload) && !hasGenericTypeArgs && !genericOverloads.empty())
        {
            filteredOverloads = genericOverloads;
            result = OverloadResolver::resolveFunctionOverload(filteredOverloads, argTypes, location, *ctx.env->getTypeCatalog());
        }

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
            std::vector<std::string> argTypeStrings;
            for (const auto& argType : argTypes)
            {
                argTypeStrings.push_back(runtimeTypes::klass::SignatureUtils::getTypeName(argType));
            }

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

        std::string resolvedName = buildMangledFunctionName(functionName, result.selectedOverload->getParameters());
        return resolvedName;
    }
}
