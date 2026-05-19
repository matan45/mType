#include "ClassDefinition.hpp"
#include <algorithm>
#include <set>
#include "../../vm/MethodSignature.hpp"

namespace runtimeTypes::klass
{
    bool ClassDefinition::isSubclassOf(const std::string& className) const
    {
        if (parentClassName == className) {
            return true;
        }

        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            if (current->getName() == className) {
                return true;
            }
            current = current->parentClass.lock();
            depth++;
        }

        return false;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findMethodInHierarchy(const std::string& methodName, size_t argCount) const
    {
        auto method = findMethod(methodName, argCount);
        if (method) {
            return method;
        }

        return traverseHierarchyForMethod([&](const ClassDefinition* classDef) {
            return classDef->findMethod(methodName, argCount);
        });
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethodInHierarchy(const std::string& methodName, size_t argCount) const
    {
        auto method = findInstanceMethod(methodName, argCount);
        if (method) {
            return method;
        }

        return traverseHierarchyForMethod([&](const ClassDefinition* classDef) {
            return classDef->findInstanceMethod(methodName, argCount);
        });
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethodInHierarchy(const std::string& methodName, size_t argCount) const
    {
        auto method = findStaticMethod(methodName, argCount);
        if (method) {
            return method;
        }

        return traverseHierarchyForMethod([&](const ClassDefinition* classDef) {
            return classDef->findStaticMethod(methodName, argCount);
        });
    }

    std::vector<std::shared_ptr<ClassDefinition>> ClassDefinition::getInheritanceChain() const
    {
        std::vector<std::shared_ptr<ClassDefinition>> chain;

        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            chain.push_back(current);
            current = current->parentClass.lock();
            depth++;
        }

        return chain;
    }

    std::vector<std::string> ClassDefinition::getUnimplementedAbstractMethods() const
    {
        std::vector<std::string> unimplemented;

        for (const auto& methodName : abstractMethods) {
            unimplemented.push_back(methodName);
        }

        auto chain = getInheritanceChain();
        for (const auto& parentClass : chain) {
            if (parentClass && parentClass->isAbstract()) {
                for (const auto& abstractMethod : parentClass->getAbstractMethods()) {
                    // An abstract method is implemented when any class in the
                    // chain between `this` and the declaring class provides a
                    // non-abstract override with the same arity.
                    bool isImplemented = false;

                    auto abstractMethodDef = parentClass->getInstanceMethod(abstractMethod);
                    if (!abstractMethodDef) {
                        continue;
                    }

                    size_t requiredArgCount = abstractMethodDef->getParameters().size();
                    // findInstanceMethod expects parameter count WITHOUT 'this'.
                    size_t argCountWithoutThis = (requiredArgCount > 0) ? requiredArgCount - 1 : 0;

                    auto implementingMethod = findInstanceMethod(abstractMethod, argCountWithoutThis);
                    if (implementingMethod && !implementingMethod->isAbstract()) {
                        isImplemented = true;
                    }

                    if (!isImplemented) {
                        for (const auto& intermediateClass : chain) {
                            if (intermediateClass == parentClass) {
                                break;
                            }

                            auto intermediateMethod = intermediateClass->findInstanceMethod(abstractMethod, argCountWithoutThis);
                            if (intermediateMethod && !intermediateMethod->isAbstract()) {
                                isImplemented = true;
                                break;
                            }
                        }
                    }

                    if (!isImplemented && std::find(unimplemented.begin(), unimplemented.end(), abstractMethod) == unimplemented.end()) {
                        unimplemented.push_back(abstractMethod);
                    }
                }
            }
        }

        return unimplemented;
    }

    bool ClassDefinition::hasAllAbstractMethodsImplemented() const
    {
        if (abstractClass) {
            return true;
        }

        auto unimplemented = getUnimplementedAbstractMethods();
        return unimplemented.empty();
    }

    std::vector<std::shared_ptr<MethodDefinition>> ClassDefinition::getAllInstanceMethodOverloadsInHierarchy(const std::string& methodName) const
    {
        std::vector<std::shared_ptr<MethodDefinition>> allOverloads;
        std::set<std::string> seenSignatures;

        // Signature key skips 'this' for instance methods so a derived override
        // dedups against the base's signature on lookup collision.
        auto getSignature = [](const std::shared_ptr<MethodDefinition>& method) {
            std::string sig = method->getName() + "(";
            const auto& params = method->getParametersWithTypes();
            size_t startIdx = (!method->isStatic() && !params.empty() && params[0].first == "this") ? 1 : 0;
            for (size_t i = startIdx; i < params.size(); ++i) {
                if (i > startIdx) sig += ",";
                sig += params[i].second.toString();
            }
            sig += ")";
            return sig;
        };

        auto localOverloads = getAllInstanceMethodOverloads(methodName);
        for (const auto& overload : localOverloads) {
            std::string sig = getSignature(overload);
            if (seenSignatures.insert(sig).second) {
                allOverloads.push_back(overload);
            }
        }

        auto parent = getParentClass();
        while (parent) {
            auto parentOverloads = parent->getAllInstanceMethodOverloads(methodName);
            for (const auto& overload : parentOverloads) {
                std::string sig = getSignature(overload);
                if (seenSignatures.insert(sig).second) {
                    allOverloads.push_back(overload);
                }
            }
            parent = parent->getParentClass();
        }

        return allOverloads;
    }

    std::vector<std::shared_ptr<MethodDefinition>> ClassDefinition::getAllStaticMethodOverloadsInHierarchy(const std::string& methodName) const
    {
        std::vector<std::shared_ptr<MethodDefinition>> allOverloads;
        std::set<std::string> seenSignatures;

        auto getSignature = [](const std::shared_ptr<MethodDefinition>& method) {
            std::string sig = method->getName() + "(";
            const auto& params = method->getParametersWithTypes();
            size_t startIdx = (!method->isStatic() && !params.empty() && params[0].first == "this") ? 1 : 0;
            for (size_t i = startIdx; i < params.size(); ++i) {
                if (i > startIdx) sig += ",";
                sig += params[i].second.toString();
            }
            sig += ")";
            return sig;
        };

        auto localOverloads = getAllStaticMethodOverloads(methodName);
        for (const auto& overload : localOverloads) {
            std::string sig = getSignature(overload);
            if (seenSignatures.insert(sig).second) {
                allOverloads.push_back(overload);
            }
        }

        auto parent = getParentClass();
        while (parent) {
            auto parentOverloads = parent->getAllStaticMethodOverloads(methodName);
            for (const auto& overload : parentOverloads) {
                std::string sig = getSignature(overload);
                if (seenSignatures.insert(sig).second) {
                    allOverloads.push_back(overload);
                }
            }
            parent = parent->getParentClass();
        }

        return allOverloads;
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethodBySignatureInHierarchy(
        const std::string& methodName,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const
    {
        auto method = findInstanceMethodBySignature(methodName, parameters);
        if (method) {
            return method;
        }

        return traverseHierarchyForMethod([&](const ClassDefinition* classDef) {
            return classDef->findInstanceMethodBySignature(methodName, parameters);
        });
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethodBySignatureInHierarchy(
        const std::string& methodName,
        const std::vector<std::pair<std::string, value::ParameterType>>& parameters) const
    {
        auto method = findStaticMethodBySignature(methodName, parameters);
        if (method) {
            return method;
        }

        return traverseHierarchyForMethod([&](const ClassDefinition* classDef) {
            return classDef->findStaticMethodBySignature(methodName, parameters);
        });
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findInstanceMethodInHierarchy(const vm::MethodSignature& signature) const
    {
        return findInstanceMethodInHierarchy(signature.getMethodName(), signature.getParameterCount());
    }

    std::shared_ptr<MethodDefinition> ClassDefinition::findStaticMethodInHierarchy(const vm::MethodSignature& signature) const
    {
        return findStaticMethodInHierarchy(signature.getMethodName(), signature.getParameterCount());
    }
}
