#include "GenericTypeManager.hpp"
#include "../../parser/TypeParser.hpp"
#include "../../runtimeTypes/klass/FieldDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../runtimeTypes/klass/ConstructorDefinition.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdint>

namespace evaluator::utils
{
    std::pair<std::string, std::vector<std::string>> GenericTypeManager::parseGenericInstantiation(
        const std::string& instantiationName)
    {
        size_t anglePos = instantiationName.find('<');
        if (anglePos == std::string::npos) {
            // Not a generic instantiation
            return {instantiationName, {}};
        }

        size_t endPos = instantiationName.rfind('>');
        if (endPos == std::string::npos || endPos <= anglePos) {
            // Malformed generic instantiation
            return {instantiationName, {}};
        }

        std::string baseName = instantiationName.substr(0, anglePos);
        std::string typeArgsString = instantiationName.substr(anglePos + 1, endPos - anglePos - 1);

        auto typeArguments = parseTypeArguments(typeArgsString);
        return {baseName, typeArguments};
    }

    std::shared_ptr<ClassDefinition> GenericTypeManager::instantiateGenericClass(
        std::shared_ptr<ClassDefinition> genericClass,
        const std::vector<std::string>& typeArguments)
    {
        if (!genericClass || !genericClass->isGeneric()) {
            return nullptr;
        }

        if (!validateTypeArguments(genericClass, typeArguments)) {
            return nullptr;
        }

        // Create instantiated class name (e.g., "Box<int>")
        std::string instantiatedName = genericClass->getBaseName() + "<";
        for (size_t i = 0; i < typeArguments.size(); ++i) {
            if (i > 0) instantiatedName += ", ";
            instantiatedName += typeArguments[i];
        }
        instantiatedName += ">";

        // Create cache key that includes the base class pointer to avoid collisions
        // between different generic classes with the same name
        std::string cacheKey = std::to_string(reinterpret_cast<uintptr_t>(genericClass.get())) + ":" + instantiatedName;

        // Check generic class instantiation cache
        static std::unordered_map<std::string, std::shared_ptr<ClassDefinition>> genericClassCache;

        auto cacheIt = genericClassCache.find(cacheKey);
        if (cacheIt != genericClassCache.end()) {
            return cacheIt->second;
        }

        // Create substitution map
        auto substitutionMap = createTypeSubstitutionMap(
            genericClass->getGenericParameters(), typeArguments);

        // Create new class definition
        auto instantiatedClass = std::make_shared<ClassDefinition>(instantiatedName);

        // Copy and substitute fields
        for (const auto& [fieldName, field] : genericClass->getInstanceFields()) {
            // Create a copy of the field with substituted type
            auto newField = std::make_shared<FieldDefinition>(
                field->getName(),
                substituteFieldType(field->getType(), substitutionMap),
                field->getValue(),
                field->isStatic(),
                field->isFinal()
            );
            instantiatedClass->addInstanceField(fieldName, newField);
        }

        for (const auto& [fieldName, field] : genericClass->getStaticFields()) {
            // Create a copy of the field with substituted type
            auto newField = std::make_shared<FieldDefinition>(
                field->getName(),
                substituteFieldType(field->getType(), substitutionMap),
                field->getValue(),
                field->isStatic(),
                field->isFinal()
            );
            instantiatedClass->addStaticField(fieldName, newField);
        }

        // Copy and substitute methods
        for (const auto& [methodName, method] : genericClass->getInstanceMethods()) {
            // Create a copy of the method with substituted types
            auto newMethod = substituteMethodTypes(method, substitutionMap);
            instantiatedClass->addInstanceMethod(methodName, newMethod);
        }

        for (const auto& [methodName, method] : genericClass->getStaticMethods()) {
            // Create a copy of the method with substituted types
            auto newMethod = substituteMethodTypes(method, substitutionMap);
            instantiatedClass->addStaticMethod(methodName, newMethod);
        }

        // Copy constructors
        for (const auto& constructor : genericClass->getConstructors()) {
            // Create a copy of the constructor with substituted parameter types
            auto newConstructor = substituteConstructorTypes(constructor, substitutionMap);
            instantiatedClass->addConstructor(newConstructor);
        }

        // Cache the instantiated class for future reuse
        genericClassCache[cacheKey] = instantiatedClass;

        return instantiatedClass;
    }

    bool GenericTypeManager::validateTypeArguments(
        std::shared_ptr<ClassDefinition> genericClass,
        const std::vector<std::string>& typeArguments)
    {
        if (!genericClass) return false;

        // Check that the number of type arguments matches the number of generic parameters
        return typeArguments.size() == genericClass->getGenericParameterCount();
    }

    bool GenericTypeManager::isGenericInstantiation(const std::string& typeName)
    {
        return typeName.find('<') != std::string::npos &&
               typeName.find('>') != std::string::npos;
    }

    std::string GenericTypeManager::getBaseClassName(const std::string& instantiationName)
    {
        size_t anglePos = instantiationName.find('<');
        if (anglePos == std::string::npos) {
            return instantiationName;
        }
        return instantiationName.substr(0, anglePos);
    }

    std::unordered_map<std::string, std::string> GenericTypeManager::createTypeSubstitutionMap(
        const std::vector<GenericTypeParameter>& genericParameters,
        const std::vector<std::string>& typeArguments)
    {
        std::unordered_map<std::string, std::string> substitutionMap;

        size_t minSize = std::min(genericParameters.size(), typeArguments.size());
        for (size_t i = 0; i < minSize; ++i) {
            substitutionMap[genericParameters[i].name] = typeArguments[i];
        }

        return substitutionMap;
    }

    std::string GenericTypeManager::substituteTypeParameters(
        const std::string& typeString,
        const std::unordered_map<std::string, std::string>& substitutionMap)
    {
        std::string result = typeString;

        // Simple substitution - replace each type parameter with its concrete type
        for (const auto& [parameter, concreteType] : substitutionMap) {
            // TODO: Implement more sophisticated substitution that handles complex types
            // For now, just do simple string replacement
            size_t pos = 0;
            while ((pos = result.find(parameter, pos)) != std::string::npos) {
                // Make sure we're replacing a whole word, not part of another identifier
                bool isWholeWord = true;
                if (pos > 0 && (std::isalnum(result[pos - 1]) || result[pos - 1] == '_')) {
                    isWholeWord = false;
                }
                if (pos + parameter.length() < result.length() &&
                    (std::isalnum(result[pos + parameter.length()]) || result[pos + parameter.length()] == '_')) {
                    isWholeWord = false;
                }

                if (isWholeWord) {
                    result.replace(pos, parameter.length(), concreteType);
                    pos += concreteType.length();
                } else {
                    pos += parameter.length();
                }
            }
        }

        return result;
    }

    std::vector<std::string> GenericTypeManager::parseTypeArguments(const std::string& typeArgsString)
    {
        std::vector<std::string> typeArgs;
        std::istringstream stream(typeArgsString);
        std::string typeArg;

        int depth = 0;
        std::string currentArg;

        for (char c : typeArgsString) {
            if (c == '<') {
                depth++;
                currentArg += c;
            } else if (c == '>') {
                depth--;
                currentArg += c;
            } else if (c == ',' && depth == 0) {
                // Top-level comma - this separates type arguments
                // Trim whitespace
                currentArg.erase(0, currentArg.find_first_not_of(" \t"));
                currentArg.erase(currentArg.find_last_not_of(" \t") + 1);

                if (!currentArg.empty()) {
                    typeArgs.push_back(currentArg);
                }
                currentArg.clear();
            } else {
                currentArg += c;
            }
        }

        // Add the last argument
        currentArg.erase(0, currentArg.find_first_not_of(" \t"));
        currentArg.erase(currentArg.find_last_not_of(" \t") + 1);
        if (!currentArg.empty()) {
            typeArgs.push_back(currentArg);
        }

        return typeArgs;
    }

    value::ValueType GenericTypeManager::substituteFieldType(
        value::ValueType originalType,
        const std::unordered_map<std::string, std::string>& substitutionMap)
    {
        // For generic methods, parameters of type OBJECT represent generic type parameters
        // The issue is that we can't know which specific type parameter this OBJECT represents
        // without more context. However, for the runtime resolution to work properly,
        // we need to preserve the OBJECT type and let the runtime resolveParameterType
        // method handle the substitution using the generic parameter information.

        // Return the original type without substitution - let runtime resolution handle it
        return originalType;
    }

    value::ValueType GenericTypeManager::convertGenericTypeToValueType(
        std::shared_ptr<ast::GenericType> genericType,
        const std::unordered_map<std::string, std::string>& substitutionMap)
    {
        if (!genericType) {
            return value::ValueType::VOID;
        }

        // Check if this is a type parameter that needs substitution
        if (genericType->isGenericParameter()) {
            std::string typeName = genericType->getGenericName();

            // Look up the substitution
            auto it = substitutionMap.find(typeName);
            if (it != substitutionMap.end()) {
                // Convert the substituted type string to ValueType
                return parser::TypeParser::stringToValueType(it->second);
            }

            // If no substitution found, default to OBJECT
            return value::ValueType::OBJECT;
        }

        // If it's a concrete type, convert it directly
        if (!genericType->isGenericParameter()) {
            return genericType->getConcreteType();
        }

        // For complex generic types (like Array<T>), we'd need more sophisticated handling
        // For now, default to OBJECT
        return value::ValueType::OBJECT;
    }

    std::shared_ptr<runtimeTypes::klass::MethodDefinition> GenericTypeManager::substituteMethodTypes(
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> originalMethod,
        const std::unordered_map<std::string, std::string>& substitutionMap)
    {
        // Substitute return type
        value::ValueType newReturnType = substituteFieldType(originalMethod->getReturnType(), substitutionMap);

        // Substitute parameter types
        auto originalParams = originalMethod->getParameters();
        std::vector<std::pair<std::string, value::ValueType>> newParams;
        newParams.reserve(originalParams.size());

        for (const auto& [paramName, paramType] : originalParams) {
            value::ValueType newParamType = substituteFieldType(paramType, substitutionMap);
            newParams.emplace_back(paramName, newParamType);
        }

        // Create new method definition with enhanced generic information
        return std::make_shared<runtimeTypes::klass::MethodDefinition>(
            originalMethod->getName(),
            newReturnType,
            newParams,
            std::vector<std::pair<std::string, value::Value>>{}, // empty arguments
            originalMethod->getBodyPtr(),
            originalMethod->isStatic(),
            originalMethod->getGenericReturnType(),   // Preserve generic return type
            originalMethod->getGenericParameters(),   // Preserve generic parameters
            originalMethod->getGenericTypeParameters(), // Preserve generic type parameter declarations
            substitutionMap                           // Store substitution map for runtime resolution
        );
    }

    std::shared_ptr<runtimeTypes::klass::ConstructorDefinition> GenericTypeManager::substituteConstructorTypes(
        std::shared_ptr<runtimeTypes::klass::ConstructorDefinition> originalConstructor,
        const std::unordered_map<std::string, std::string>& substitutionMap)
    {
        // Get the original parameters and substitute their types
        auto originalParams = originalConstructor->getParameters();
        std::vector<std::pair<std::string, value::ValueType>> newParams;
        newParams.reserve(originalParams.size());

        for (const auto& [paramName, paramType] : originalParams) {
            value::ValueType newParamType = substituteFieldType(paramType, substitutionMap);
            newParams.emplace_back(paramName, newParamType);
        }

        // Create new constructor definition with substituted parameter types
        return std::make_shared<runtimeTypes::klass::ConstructorDefinition>(
            newParams,
            originalConstructor->getBodyPtr()
        );
    }

    std::shared_ptr<runtimeTypes::klass::MethodDefinition> GenericTypeManager::instantiateStaticGenericMethod(
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> genericMethod,
        const std::vector<std::string>& typeArguments)
    {
        if (!genericMethod->hasGenericInformation()) {
            // Method is not generic, return as-is
            return genericMethod;
        }

        // Get the actual generic type parameters (like <T>, <K,V>)
        const auto& genericParams = genericMethod->getGenericTypeParameters();

        // Create type substitution map
        auto substitutionMap = createTypeSubstitutionMap(
            genericParams,
            typeArguments
        );

        // Create specialized method with type substitution
        return substituteMethodTypes(genericMethod, substitutionMap);
    }

    std::string GenericTypeManager::createStaticMethodSignatureKey(
        const std::string& className,
        const std::string& methodName,
        const std::vector<std::string>& typeArguments)
    {
        std::string key = className + "::" + methodName;

        if (!typeArguments.empty()) {
            key += "<";
            for (size_t i = 0; i < typeArguments.size(); ++i) {
                if (i > 0) key += ",";
                key += typeArguments[i];
            }
            key += ">";
        }

        return key;
    }

    bool GenericTypeManager::validateStaticMethodTypeArguments(
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> genericMethod,
        const std::vector<std::string>& typeArguments)
    {
        if (!genericMethod->hasGenericInformation()) {
            // Non-generic method should not have type arguments
            return typeArguments.empty();
        }

        // Check if type argument count matches generic type parameter count
        const auto& genericTypeParams = genericMethod->getGenericTypeParameters();
        if (typeArguments.size() != genericTypeParams.size()) {
            return false;
        }

        // TODO: Add constraint checking when bounds are implemented
        // For now, just check that all type arguments are non-empty
        for (const auto& typeArg : typeArguments) {
            if (typeArg.empty()) {
                return false;
            }
        }

        return true;
    }
}