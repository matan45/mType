#include "ClassDefinition.hpp"
#include "InterfaceRegistry.hpp"
#include "InterfaceDefinition.hpp"
#include "../../types/TypeSubstitutionService.hpp"

namespace runtimeTypes::klass
{
    std::string ClassDefinition::getGenericClassName() const
    {
        if (!isGeneric()) {
            return getName();
        }

        std::string result = getName() + "<";
        for (size_t i = 0; i < genericParameters.size(); ++i) {
            if (i > 0) result += ", ";
            result += genericParameters[i].name;
        }
        result += ">";
        return result;
    }

    bool ClassDefinition::hasGenericParameter(const std::string& paramName) const
    {
        for (const auto& param : genericParameters) {
            if (param.name == paramName) {
                return true;
            }
        }
        return false;
    }

    bool ClassDefinition::implementsInterface(const std::string& interfaceName) const
    {
        // Direct interface match only; transitive checks require the registry overload.
        for (const auto& implementedInterface : implementedInterfaces) {
            std::string baseImplementedName = extractBaseTypeName(implementedInterface);

            if (baseImplementedName == interfaceName) {
                return true;
            }
        }

        return false;
    }

    bool ClassDefinition::implementsInterface(const std::string& interfaceName, std::shared_ptr<InterfaceRegistry> registry) const
    {
        std::string normalizedExpected = normalizeGenericTypeName(interfaceName);

        for (const auto& implementedInterface : implementedInterfaces) {
            std::string normalizedImplemented = normalizeGenericTypeName(implementedInterface);

            if (normalizedImplemented == normalizedExpected) {
                return true;
            }

            // Base-name fallback so non-generic queries match generic implementations.
            std::string baseImplementedName = extractBaseTypeName(implementedInterface);
            std::string baseExpectedName = extractBaseTypeName(interfaceName);
            if (baseImplementedName == baseExpectedName) {
                return true;
            }
        }

        std::unordered_set<std::string> visited;
        return implementsInterfaceTransitive(interfaceName, visited, 0, registry);
    }

    std::string ClassDefinition::extractBaseTypeName(const std::string& typeName)
    {
        size_t anglePos = typeName.find('<');
        return (anglePos != std::string::npos) ? typeName.substr(0, anglePos) : typeName;
    }

    std::string ClassDefinition::normalizeGenericTypeName(const std::string& typeName)
    {
        // "Function<Int, String>" -> "Function<Int,String>"
        std::string normalized;
        normalized.reserve(typeName.size());

        for (size_t i = 0; i < typeName.size(); ++i) {
            char c = typeName[i];

            if (c == ' ' && i > 0 && typeName[i - 1] == ',') {
                continue;
            }

            normalized += c;
        }

        return normalized;
    }

    bool ClassDefinition::checkDirectInterfaceMatch(const std::string& interfaceName,
                                                     const std::string& implementedInterface,
                                                     std::unordered_set<std::string>& visited,
                                                     std::shared_ptr<InterfaceRegistry> registry) const
    {
        std::string baseImplementedName = extractBaseTypeName(implementedInterface);

        if (visited.find(baseImplementedName) != visited.end()) {
            return false;
        }

        visited.insert(baseImplementedName);

        auto interfaceDef = registry->findInterface(baseImplementedName);
        if (interfaceDef) {
            const auto& extendedInterfaces = interfaceDef->getExtendedInterfaces();
            for (const auto& extendedInterface : extendedInterfaces) {
                std::string baseExtendedName = extractBaseTypeName(extendedInterface);

                if (baseExtendedName == interfaceName) {
                    visited.erase(baseImplementedName);
                    return true;
                }

                auto extendedInterfaceDef = registry->findInterface(baseExtendedName);
                if (extendedInterfaceDef) {
                    const auto& furtherExtended = extendedInterfaceDef->getExtendedInterfaces();
                    for (const auto& furtherInterface : furtherExtended) {
                        std::string furtherBaseName = extractBaseTypeName(furtherInterface);

                        if (furtherBaseName == interfaceName) {
                            visited.erase(baseImplementedName);
                            return true;
                        }
                    }
                }
            }
        }

        visited.erase(baseImplementedName);
        return false;
    }

    bool ClassDefinition::implementsInterfaceTransitive(const std::string& interfaceName,
                                                       std::unordered_set<std::string>& visited,
                                                       int depth,
                                                       std::shared_ptr<InterfaceRegistry> registry) const
    {
        if (depth > MAX_INTERFACE_DEPTH) {
            return false;
        }

        if (!registry) {
            return false;
        }

        for (const auto& implementedInterface : implementedInterfaces) {
            if (checkDirectInterfaceMatch(interfaceName, implementedInterface, visited, registry)) {
                return true;
            }
        }

        return false;
    }

    bool ClassDefinition::hasAnnotation(const std::string& annotationName) const
    {
        for (const auto& annotation : annotations)
        {
            if (annotation->getName() == annotationName)
            {
                return true;
            }
        }
        return false;
    }

    std::shared_ptr<ast::nodes::annotations::AnnotationNode> ClassDefinition::getAnnotation(const std::string& annotationName) const
    {
        for (const auto& annotation : annotations)
        {
            if (annotation->getName() == annotationName)
            {
                return annotation;
            }
        }
        return nullptr;
    }

    ::types::UnifiedTypePtr ClassDefinition::resolveTypeInContext(const ::types::UnifiedTypePtr& type) const
    {
        if (!type)
        {
            return nullptr;
        }

        if (inheritanceSubstitutionChain.empty())
        {
            if (parentTypeSubstitutionMap.empty())
            {
                return type;
            }

            // String-based substitution map → UnifiedType map for legacy callers.
            ::types::TypeSubstitutionMap legacyMap;
            for (const auto& [param, concrete] : parentTypeSubstitutionMap)
            {
                legacyMap[param] = ::types::UnifiedType::classType(concrete);
            }

            ::types::TypeSubstitutionService service;
            return service.substitute(type, legacyMap);
        }

        ::types::TypeSubstitutionService service;
        auto composedMap = service.composeChain(inheritanceSubstitutionChain);
        return service.substitute(type, composedMap);
    }
}
