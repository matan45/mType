#include "ClassDefinition.hpp"
#include <algorithm>
#include "SignatureUtils.hpp"
#include "../../vm/MethodSignature.hpp"
#include "../../types/TypeSubstitutionService.hpp"

namespace runtimeTypes::klass
{
    std::shared_ptr<MethodDefinition> ClassDefinition::getMethod(const std::string& methodName) const
    {
        auto method = getInstanceMethod(methodName);
        if (method) {
            return method;
        }
        return getStaticMethod(methodName);
    }

    std::shared_ptr<ConstructorDefinition> ClassDefinition::getConstructor() const
    {
        return constructors.empty() ? nullptr : constructors[0];
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findMethod(const std::string& methodName, size_t argCount) const
    {
        auto instanceMethod = findInstanceMethod(methodName, argCount);
        if (instanceMethod) {
            return instanceMethod;
        }

        auto staticMethod = findStaticMethod(methodName, argCount);
        if (staticMethod) {
            return staticMethod;
        }

        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::getStaticMethod(const std::string& methodName) const
    {
        auto it = staticMethods.find(methodName);
        if (it != staticMethods.end() && !it->second.empty()) {
            return it->second[0];
        }
        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::getInstanceMethod(const std::string& methodName) const
    {
        auto it = instanceMethods.find(methodName);
        if (it != instanceMethods.end() && !it->second.empty()) {
            return it->second[0];
        }
        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethod(const std::string& methodName, size_t argCount) const
    {
        auto it = staticMethods.find(methodName);
        if (it != staticMethods.end()) {
            for (const auto& method : it->second) {
                if (method && method->getParameters().size() == argCount) {
                    return method;
                }
            }
        }
        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethod(const std::string& methodName, size_t argCount) const
    {
        // User-declared methods take precedence; check them first.
        auto it = instanceMethods.find(methodName);
        if (it != instanceMethods.end()) {
            for (const auto& method : it->second) {
                if (method) {
                    size_t methodParamCount = method->getParameters().size();
                    if (!method->isStatic() && methodParamCount > 0) {
                        methodParamCount--; // exclude 'this'
                    }
                    if (methodParamCount == argCount) {
                        return method;
                    }
                }
            }
        }
        // MYT-274: fall through to compiler-synthesized methods (only used
        // when there's no user-declared override of the same signature).
        auto sit = syntheticInstanceMethods.find(methodName);
        if (sit != syntheticInstanceMethods.end()) {
            for (const auto& method : sit->second) {
                if (method) {
                    size_t methodParamCount = method->getParameters().size();
                    if (!method->isStatic() && methodParamCount > 0) {
                        methodParamCount--;
                    }
                    if (methodParamCount == argCount) {
                        return method;
                    }
                }
            }
        }
        return nullptr;
    }

    ClassDefinition::MethodLookupResult ClassDefinition::findInstanceMethodCached(
        const std::string& methodName, size_t argCount) const
    {
        std::string cacheKey = methodName + "/" + std::to_string(argCount);

        auto cacheIt = instanceMethodCache.find(cacheKey);
        if (cacheIt != instanceMethodCache.end()) {
            return cacheIt->second;
        }

        MethodLookupResult result;

        auto method = findInstanceMethod(methodName, argCount);
        if (method) {
            result.method = method;
            result.definingClassName = getName();
            auto signature = vm::MethodSignature::fromMethodDefinition(method.get());
            result.qualifiedName = signature.toMangledName(result.definingClassName, false);
            instanceMethodCache[cacheKey] = result;
            return result;
        }

        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            method = current->findInstanceMethod(methodName, argCount);
            if (method) {
                result.method = method;
                result.definingClassName = current->getName();
                auto signature = vm::MethodSignature::fromMethodDefinition(method.get());
                result.qualifiedName = signature.toMangledName(result.definingClassName, false);
                instanceMethodCache[cacheKey] = result;
                return result;
            }
            current = current->parentClass.lock();
            depth++;
        }

        result.method = nullptr;
        result.definingClassName = "";
        result.qualifiedName = "";
        instanceMethodCache[cacheKey] = result;
        return result;
    }

    ClassDefinition::MethodLookupResult ClassDefinition::findInstanceMethodForCallNameCached(
        const std::string& callName,
        size_t argCount) const
    {
        std::string simpleMethodName;
        std::string typeSignature;

        if (SignatureUtils::hasSignature(callName))
        {
            std::string ignoredClassName;
            if (!SignatureUtils::parseQualifiedName(
                    callName, ignoredClassName, simpleMethodName, typeSignature))
            {
                SignatureUtils::parseFunctionName(callName, simpleMethodName, typeSignature);
            }

            const std::string cacheKey = "sig:" + simpleMethodName + "/" + typeSignature;
            auto cacheIt = instanceMethodCache.find(cacheKey);
            if (cacheIt != instanceMethodCache.end())
            {
                return cacheIt->second;
            }

            MethodLookupResult result;

            std::vector<std::string> candidateSignatures;
            auto addCandidateSignature = [&candidateSignatures](const std::string& signature)
            {
                if (std::find(candidateSignatures.begin(), candidateSignatures.end(), signature) ==
                    candidateSignatures.end())
                {
                    candidateSignatures.push_back(signature);
                }
            };
            auto addSignatureWithNullableErasure =
                [&addCandidateSignature](const std::string& signature)
            {
                auto parts = SignatureUtils::parseTypeSignature(signature);
                bool changed = false;
                for (auto& part : parts)
                {
                    if (!part.empty() && part.back() == '?')
                    {
                        part.pop_back();
                        changed = true;
                    }
                }

                if (changed)
                {
                    addCandidateSignature(SignatureUtils::generateTypeSignatureFromNames(parts));
                }
            };

            std::unordered_map<std::string, std::string> substitutions = parentTypeSubstitutionMap;
            if (!inheritanceSubstitutionChain.empty())
            {
                ::types::TypeSubstitutionService substitutionService;
                auto composedMap = substitutionService.composeChain(inheritanceSubstitutionChain);
                for (const auto& [param, concreteType] : composedMap)
                {
                    if (concreteType)
                    {
                        substitutions[param] = concreteType->toString();
                    }
                }
            }

            if (!substitutions.empty() && !typeSignature.empty())
            {
                ::types::TypeSubstitutionService substitutionService;
                auto parts = SignatureUtils::parseTypeSignature(typeSignature);
                bool changed = false;
                for (auto& part : parts)
                {
                    std::string resolved = substitutionService.resolveGenericType(part, substitutions);
                    if (resolved != part)
                    {
                        changed = true;
                        part = std::move(resolved);
                    }
                }

                if (changed)
                {
                    std::string substitutedSignature =
                        SignatureUtils::generateTypeSignatureFromNames(parts);
                    addCandidateSignature(substitutedSignature);
                    addSignatureWithNullableErasure(substitutedSignature);
                }
            }

            addSignatureWithNullableErasure(typeSignature);
            addCandidateSignature(typeSignature);

            auto assignResult = [&result](std::shared_ptr<MethodDefinition> method,
                                          const std::string& definingClassName)
            {
                result.method = std::move(method);
                result.definingClassName = definingClassName;
                auto signature = vm::MethodSignature::fromMethodDefinition(result.method.get());
                result.qualifiedName = signature.toMangledName(result.definingClassName, false);
            };

            for (const auto& candidateSignature : candidateSignatures)
            {
                auto method = findInstanceMethodByTypeSignature(simpleMethodName, candidateSignature);
                if (method && !method->isAbstract())
                {
                    assignResult(method, getName());
                    instanceMethodCache[cacheKey] = result;
                    return result;
                }

                auto current = parentClass.lock();
                int depth = 0;
                while (current && depth < MAX_INHERITANCE_DEPTH)
                {
                    method = current->findInstanceMethodByTypeSignature(simpleMethodName, candidateSignature);
                    if (method && !method->isAbstract())
                    {
                        assignResult(method, current->getName());
                        instanceMethodCache[cacheKey] = result;
                        return result;
                    }
                    current = current->parentClass.lock();
                    depth++;
                }
            }

            size_t arityMatches = 0;
            auto countArityMatches = [&](const ClassDefinition* classDef)
            {
                if (!classDef) return;
                auto it = classDef->instanceMethods.find(simpleMethodName);
                if (it == classDef->instanceMethods.end()) return;

                for (const auto& method : it->second)
                {
                    if (!method || method->isAbstract()) continue;
                    size_t methodParamCount = method->getParameters().size();
                    if (!method->isStatic() && methodParamCount > 0)
                    {
                        methodParamCount--;
                    }
                    if (methodParamCount == argCount)
                    {
                        arityMatches++;
                    }
                }
            };

            countArityMatches(this);
            auto current = parentClass.lock();
            int depth = 0;
            while (current && depth < MAX_INHERITANCE_DEPTH && arityMatches <= 1)
            {
                countArityMatches(current.get());
                current = current->parentClass.lock();
                depth++;
            }

            auto fallbackResult = (arityMatches == 1)
                ? findInstanceMethodCached(simpleMethodName, argCount)
                : MethodLookupResult{};
            if (fallbackResult.method && !fallbackResult.method->isAbstract())
            {
                instanceMethodCache[cacheKey] = fallbackResult;
                return fallbackResult;
            }

            instanceMethodCache[cacheKey] = result;
            return result;
        }

        simpleMethodName = SignatureUtils::extractSimpleName(callName);
        return findInstanceMethodCached(simpleMethodName, argCount);
    }

    std::vector<std::shared_ptr<MethodDefinition>> ClassDefinition::getAllInstanceMethodOverloads(const std::string& methodName) const
    {
        auto it = instanceMethods.find(methodName);
        if (it != instanceMethods.end()) {
            return it->second;
        }
        return {};
    }

    std::vector<std::shared_ptr<MethodDefinition>> ClassDefinition::getAllStaticMethodOverloads(const std::string& methodName) const
    {
        auto it = staticMethods.find(methodName);
        if (it != staticMethods.end()) {
            return it->second;
        }
        return {};
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethodBySignature(
        const std::string& methodName,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const
    {
        auto searchMap = [&](const std::unordered_map<std::string,
            std::vector<std::shared_ptr<MethodDefinition>>>& methods)
            -> std::shared_ptr<MethodDefinition>
        {
            auto it = methods.find(methodName);
            if (it == methods.end()) return nullptr;
            for (const auto& method : it->second) {
                if (!method) continue;
                const auto& methodParams = method->getParametersWithTypes();
                std::vector<std::pair<std::string, value::ParameterType>> methodParamsWithoutThis;
                for (const auto& param : methodParams) {
                    if (param.first != "this") {
                        methodParamsWithoutThis.push_back(param);
                    }
                }
                if (methodParamsWithoutThis.size() != parameters.size()) continue;
                bool allMatch = true;
                for (size_t i = 0; i < parameters.size(); ++i) {
                    if (!(methodParamsWithoutThis[i].second == parameters[i].second)) {
                        allMatch = false;
                        break;
                    }
                }
                if (allMatch) return method;
            }
            return nullptr;
        };

        // User-declared methods take precedence; MYT-274 synthetic methods are a fallthrough.
        if (auto m = searchMap(instanceMethods)) return m;
        return searchMap(syntheticInstanceMethods);
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethodBySignature(
        const std::string& methodName,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const
    {
        auto it = staticMethods.find(methodName);
        if (it == staticMethods.end()) {
            return nullptr;
        }

        for (const auto& method : it->second) {
            if (!method) continue;

            const auto& methodParams = method->getParametersWithTypes();
            if (methodParams.size() != parameters.size()) {
                continue;
            }

            bool allMatch = true;
            for (size_t i = 0; i < parameters.size(); ++i) {
                if (!(methodParams[i].second == parameters[i].second)) {
                    allMatch = false;
                    break;
                }
            }

            if (allMatch) {
                return method;
            }
        }

        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethodByTypeSignature(
        const std::string& methodName,
        const std::string& typeSignature) const
    {
        // User-declared methods take precedence; MYT-274 synthetic methods are a fallthrough.
        auto it = instanceMethods.find(methodName);
        if (it != instanceMethods.end()) {
            for (const auto& method : it->second) {
                if (!method) continue;
                if (method->getTypeSignature() == typeSignature) {
                    return method;
                }
            }
        }
        auto sit = syntheticInstanceMethods.find(methodName);
        if (sit != syntheticInstanceMethods.end()) {
            for (const auto& method : sit->second) {
                if (!method) continue;
                if (method->getTypeSignature() == typeSignature) {
                    return method;
                }
            }
        }
        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethodByTypeSignature(
        const std::string& methodName,
        const std::string& typeSignature) const
    {
        auto it = staticMethods.find(methodName);
        if (it == staticMethods.end()) {
            return nullptr;
        }

        for (const auto& method : it->second) {
            if (!method) continue;

            if (method->getTypeSignature() == typeSignature) {
                return method;
            }
        }

        return nullptr;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethod(const vm::MethodSignature& signature) const
    {
        return findInstanceMethod(signature.getMethodName(), signature.getParameterCount());
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethod(const vm::MethodSignature& signature) const
    {
        return findStaticMethod(signature.getMethodName(), signature.getParameterCount());
    }
}
