#include "GenericTypeManager.hpp"
#include "../../parser/TypeParser.hpp"
#include "../../runtimeTypes/klass/FieldDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../../types/TypeRegistry.hpp"
#include "../../types/TypeConversionUtils.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <mutex>

namespace evaluator::utils
{
    std::pair<std::string, std::vector<std::string>> GenericTypeManager::parseGenericInstantiation(
        const std::string& instantiationName)
    {
        size_t anglePos = instantiationName.find('<');
        if (anglePos == std::string::npos)
        {
            // Not a generic instantiation
            return {instantiationName, {}};
        }

        size_t endPos = instantiationName.rfind('>');
        if (endPos == std::string::npos || endPos <= anglePos)
        {
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
        if (!genericClass || !genericClass->isGeneric())
        {
            return nullptr;
        }

        if (!validateTypeArguments(genericClass, typeArguments))
        {
            return nullptr;
        }

        // Create instantiated class name (e.g., "Box<int>")
        std::string instantiatedName = genericClass->getBaseName() + "<";
        for (size_t i = 0; i < typeArguments.size(); ++i)
        {
            if (i > 0) instantiatedName += ", ";
            instantiatedName += typeArguments[i];
        }
        instantiatedName += ">";

        // OPTIMIZATION: Check fast cache first for common patterns
        if (isCommonPattern(genericClass->getName(), typeArguments)) {
            auto fastCacheData = getFastGenericCache();
            std::mutex& fastCacheMutex = fastCacheData.first;
            auto& fastCache = fastCacheData.second;

            FastCacheKey fastKey(genericClass->getName(), typeArguments);

            {
                std::lock_guard<std::mutex> lock(fastCacheMutex);
                auto fastIt = fastCache.find(fastKey);
                if (fastIt != fastCache.end()) {
                    return fastIt->second;
                }
            }

            // Not in fast cache, proceed to create and cache in both levels
        }

        // Fallback to comprehensive cache key for complex/uncommon patterns
        std::string cacheKey = createComprehensiveCacheKey(genericClass, typeArguments);

        // Check generic class instantiation cache with thread safety
        // Use function-local static to ensure same variables across method calls
        auto cacheData = getGenericClassCache();
        std::mutex& cacheMutex = cacheData.first;
        std::unordered_map<std::string, std::shared_ptr<ClassDefinition>>& cache = cacheData.second;

        {
            std::lock_guard<std::mutex> lock(cacheMutex);

            auto cacheIt = cache.find(cacheKey);
            if (cacheIt != cache.end())
            {
                return cacheIt->second;
            }
        }


        // Create substitution map
        auto substitutionMap = createTypeSubstitutionMap(
            genericClass->getGenericParameters(), typeArguments);

        // Create new class definition
        auto instantiatedClass = std::make_shared<ClassDefinition>(instantiatedName);

        // Copy and substitute fields
        for (const auto& [fieldName, field] : genericClass->getInstanceFields())
        {
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

        for (const auto& [fieldName, field] : genericClass->getStaticFields())
        {
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
        for (const auto& [methodName, method] : genericClass->getInstanceMethods())
        {
            // Create a copy of the method with substituted types
            auto newMethod = substituteMethodTypes(method, substitutionMap);
            instantiatedClass->addInstanceMethod(methodName, newMethod);
        }

        for (const auto& [methodName, method] : genericClass->getStaticMethods())
        {
            // Create a copy of the method with substituted types
            auto newMethod = substituteMethodTypes(method, substitutionMap);
            instantiatedClass->addStaticMethod(methodName, newMethod);
        }

        // Copy constructors
        for (const auto& constructor : genericClass->getConstructors())
        {
            // Create a copy of the constructor with substituted parameter types
            auto newConstructor = substituteConstructorTypes(constructor, substitutionMap);
            instantiatedClass->addConstructor(newConstructor);
        }

        // Cache the instantiated class for future reuse in both caches
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            cache[cacheKey] = instantiatedClass;
        }

        // OPTIMIZATION: Also cache in fast cache if common pattern
        if (isCommonPattern(genericClass->getName(), typeArguments)) {
            auto fastCacheData = getFastGenericCache();
            std::mutex& fastCacheMutex = fastCacheData.first;
            auto& fastCache = fastCacheData.second;

            FastCacheKey fastKey(genericClass->getName(), typeArguments);

            std::lock_guard<std::mutex> fastLock(fastCacheMutex);
            fastCache[fastKey] = instantiatedClass;
        }

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
        if (anglePos == std::string::npos)
        {
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
        for (size_t i = 0; i < minSize; ++i)
        {
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
        for (const auto& [parameter, concreteType] : substitutionMap)
        {
            // TODO: Implement more sophisticated substitution that handles complex types
            // For now, just do simple string replacement
            size_t pos = 0;
            while ((pos = result.find(parameter, pos)) != std::string::npos)
            {
                // Make sure we're replacing a whole word, not part of another identifier
                bool isWholeWord = true;
                if (pos > 0 && (std::isalnum(result[pos - 1]) || result[pos - 1] == '_'))
                {
                    isWholeWord = false;
                }
                if (pos + parameter.length() < result.length() &&
                    (std::isalnum(result[pos + parameter.length()]) || result[pos + parameter.length()] == '_'))
                {
                    isWholeWord = false;
                }

                if (isWholeWord)
                {
                    result.replace(pos, parameter.length(), concreteType);
                    pos += concreteType.length();
                }
                else
                {
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

        for (char c : typeArgsString)
        {
            if (c == '<')
            {
                depth++;
                currentArg += c;
            }
            else if (c == '>')
            {
                depth--;
                currentArg += c;
            }
            else if (c == ',' && depth == 0)
            {
                // Top-level comma - this separates type arguments
                // Trim whitespace
                currentArg.erase(0, currentArg.find_first_not_of(" \t"));
                currentArg.erase(currentArg.find_last_not_of(" \t") + 1);

                if (!currentArg.empty())
                {
                    typeArgs.push_back(currentArg);
                }
                currentArg.clear();
            }
            else
            {
                currentArg += c;
            }
        }

        // Add the last argument
        currentArg.erase(0, currentArg.find_first_not_of(" \t"));
        currentArg.erase(currentArg.find_last_not_of(" \t") + 1);
        if (!currentArg.empty())
        {
            typeArgs.push_back(currentArg);
        }

        return typeArgs;
    }

    value::ValueType GenericTypeManager::substituteFieldType(
        value::ValueType originalType,
        const std::unordered_map<std::string, std::string>& substitutionMap)
    {
        // If the original type is OBJECT, it likely represents a generic type parameter
        // that needs to be substituted based on the substitution map
        if (originalType == value::ValueType::OBJECT) {
            // For single type parameter generics, check if we have a substitution
            if (substitutionMap.size() == 1) {
                auto it = substitutionMap.begin();
                const std::string& substitutedTypeName = it->second;

                // Use TypeRegistry to convert the substituted type name to ValueType
                auto& registry = types::getGlobalTypeRegistry();
                if (registry.hasType(substitutedTypeName)) {
                    if (registry.isArrayType(substitutedTypeName)) {
                        return value::ValueType::ARRAY;
                    }
                    return registry.getValueType(substitutedTypeName);
                }
            }

            // If we can't determine the substitution, preserve OBJECT type
            return originalType;
        }

        // For non-OBJECT types, return as-is (they don't need substitution)
        return originalType;
    }

    value::ValueType GenericTypeManager::convertGenericTypeToValueType(
        std::shared_ptr<ast::GenericType> genericType,
        const std::unordered_map<std::string, std::string>& substitutionMap)
    {
        try {
            // Use the enhanced type conversion utility with proper error handling
            types::TypeConversionContext context("GenericTypeManager::convertGenericTypeToValueType",
                                                "generic type conversion");

            return types::TypeConversionUtils::convertWithContext(
                genericType, substitutionMap, context);

        } catch (const types::TypeConversionException&) {
            // Log detailed error information if needed
            // For now, maintain backward compatibility by returning OBJECT
            // In the future, this could be configured to throw or log errors
            return value::ValueType::OBJECT;

        } catch (...) {
            // Handle unexpected errors gracefully
            return value::ValueType::OBJECT;
        }
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

        for (const auto& [paramName, paramType] : originalParams)
        {
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
            originalMethod->getGenericReturnType(), // Preserve generic return type
            originalMethod->getGenericParameters(), // Preserve generic parameters
            originalMethod->getGenericTypeParameters(), // Preserve generic type parameter declarations
            substitutionMap // Store substitution map for runtime resolution
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

        for (const auto& [paramName, paramType] : originalParams)
        {
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
        if (!genericMethod->hasGenericInformation())
        {
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

        if (!typeArguments.empty())
        {
            key += "<";
            for (size_t i = 0; i < typeArguments.size(); ++i)
            {
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
        if (!genericMethod->hasGenericInformation())
        {
            // Non-generic method should not have type arguments
            return typeArguments.empty();
        }

        // Check if type argument count matches generic type parameter count
        const auto& genericTypeParams = genericMethod->getGenericTypeParameters();
        if (typeArguments.size() != genericTypeParams.size())
        {
            return false;
        }

        // Validate type arguments for unbounded generics
        // Constraint checking would go here when bounded generics are implemented
        for (const auto& typeArg : typeArguments)
        {
            if (typeArg.empty())
            {
                return false;
            }
        }

        return true;
    }

    std::pair<std::mutex&, std::unordered_map<std::string, std::shared_ptr<ClassDefinition>>&>
        GenericTypeManager::getGenericClassCache()
    {
        // Function-local static ensures same variables across all calls
        static std::mutex genericClassCacheMutex;
        static std::unordered_map<std::string, std::shared_ptr<ClassDefinition>> genericClassCache;

        return {genericClassCacheMutex, genericClassCache};
    }

    void GenericTypeManager::clearGenericClassCache()
    {
        // Clear both regular and fast caches
        auto cacheData = getGenericClassCache();
        std::mutex& cacheMutex = cacheData.first;
        std::unordered_map<std::string, std::shared_ptr<ClassDefinition>>& cache = cacheData.second;

        auto fastCacheData = getFastGenericCache();
        std::mutex& fastCacheMutex = fastCacheData.first;
        auto& fastCache = fastCacheData.second;

        // Use lock ordering to prevent deadlocks
        std::lock_guard<std::mutex> lock1(cacheMutex);
        std::lock_guard<std::mutex> lock2(fastCacheMutex);

        cache.clear();
        fastCache.clear();
    }

    std::string GenericTypeManager::createComprehensiveCacheKey(
        std::shared_ptr<ClassDefinition> genericClass,
        const std::vector<std::string>& typeArguments)
    {
        if (!genericClass)
        {
            return "";
        }

        // Build comprehensive key: "className[genericParams]<typeArgs>"
        std::string cacheKey = genericClass->getName();

        // Add generic parameter signature to prevent collisions between different generic classes
        // Even if class names are same, different generic parameter signatures make them different
        if (genericClass->isGeneric())
        {
            cacheKey += "[";
            const auto& genericParams = genericClass->getGenericParameters();
            for (size_t i = 0; i < genericParams.size(); ++i)
            {
                if (i > 0) cacheKey += ",";
                cacheKey += genericParams[i].name;
            }
            cacheKey += "]";
        }

        // Add the concrete type arguments
        cacheKey += "<";
        for (size_t i = 0; i < typeArguments.size(); ++i)
        {
            if (i > 0) cacheKey += ",";
            cacheKey += typeArguments[i];
        }
        cacheKey += ">";

        return cacheKey;
    }

    GenericTypeManager::FastCacheKey::FastCacheKey(const std::string& name, const std::vector<std::string>& args)
        : className(name), typeArgs(args)
    {
        // Pre-compute hash for O(1) comparison
        std::hash<std::string> hasher;
        hashValue = hasher(className);

        // Combine with type argument hashes using a simple but effective method
        for (const auto& arg : typeArgs) {
            hashValue ^= hasher(arg) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
        }
    }

    std::pair<std::mutex&, std::unordered_map<GenericTypeManager::FastCacheKey, std::shared_ptr<ClassDefinition>, GenericTypeManager::FastCacheKeyHasher>&>
        GenericTypeManager::getFastGenericCache()
    {
        // Separate fast cache with optimized keys
        static std::mutex fastGenericCacheMutex;
        static std::unordered_map<FastCacheKey, std::shared_ptr<ClassDefinition>, FastCacheKeyHasher> fastGenericCache;

        return {fastGenericCacheMutex, fastGenericCache};
    }

    bool GenericTypeManager::isCommonPattern(const std::string& className, const std::vector<std::string>& typeArguments)
    {
        // Define common patterns that benefit from fast caching
        static const std::unordered_set<std::string> commonClasses = {
            "List", "Array", "Vector",           // Single-parameter containers
            "Map", "HashMap", "Dictionary",      // Key-value containers
            "Set", "HashSet",                    // Set containers
            "Stack", "Queue",                    // LIFO/FIFO containers
            "Optional", "Nullable",              // Wrapper types
            "Pair", "Tuple"                      // Multi-value types
        };

        // Check if this is a commonly instantiated class
        if (commonClasses.find(className) == commonClasses.end()) {
            return false;
        }

        // Additional criteria for fast cache eligibility:
        // 1. Reasonable type argument count (avoid extremely complex instantiations)
        if (typeArguments.size() > 4) {
            return false;
        }

        // 2. Avoid deeply nested generics in type arguments (these are complex)
        for (const auto& arg : typeArguments) {
            // Count angle brackets to detect nesting depth
            size_t depth = 0;
            for (char c : arg) {
                if (c == '<') depth++;
                if (c == '>') depth--;
                if (depth > 2) {  // Max nesting depth for fast cache
                    return false;
                }
            }
        }

        // 3. Avoid extremely long type argument names (indicates complexity)
        for (const auto& arg : typeArguments) {
            if (arg.length() > 50) {
                return false;
            }
        }

        return true;
    }

}
