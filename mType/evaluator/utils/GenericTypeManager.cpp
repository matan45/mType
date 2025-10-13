#include "GenericTypeManager.hpp"
#include "../../parser/TypeParser.hpp"
#include "../../runtimeTypes/klass/FieldDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../../runtimeTypes/klass/InterfaceRegistry.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
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
        }

        // Fallback to comprehensive cache key for complex/uncommon patterns
        std::string cacheKey = createComprehensiveCacheKey(genericClass, typeArguments);

        // Check generic class instantiation cache with thread safety
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

        // Create instantiated class
        std::string instantiatedName = createInstantiatedClassName(genericClass, typeArguments);
        auto instantiatedClass = std::make_shared<ClassDefinition>(instantiatedName);

        // Create AST-based substitution map
        auto substitutionMap = createASTSubstitutionMap(
            genericClass->getGenericParameters(), typeArguments);

        // Copy parent class link if exists
        auto parentClass = genericClass->getParentClass();
        if (parentClass) {
            instantiatedClass->setParentClass(parentClass);
            instantiatedClass->setParentClassName(genericClass->getParentClassName());
        }

        // Copy and substitute all members
        copyAndSubstituteFields(genericClass, instantiatedClass, substitutionMap);
        copyAndSubstituteMethods(genericClass, instantiatedClass, substitutionMap);
        copyAndSubstituteConstructors(genericClass, instantiatedClass, substitutionMap);

        // Cache the instantiated class for future reuse
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

    std::string GenericTypeManager::createInstantiatedClassName(
        std::shared_ptr<ClassDefinition> genericClass,
        const std::vector<std::string>& typeArguments)
    {
        std::string name = genericClass->getBaseName() + "<";
        for (size_t i = 0; i < typeArguments.size(); ++i)
        {
            if (i > 0) name += ", ";
            name += typeArguments[i];
        }
        name += ">";
        return name;
    }

    void GenericTypeManager::copyAndSubstituteFields(
        std::shared_ptr<ClassDefinition> source,
        std::shared_ptr<ClassDefinition> target,
        const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap)
    {
        // Copy instance fields with AST-based type substitution
        for (const auto& [fieldName, field] : source->getInstanceFields())
        {
            value::ValueType newFieldType = field->getType();
            std::shared_ptr<ast::GenericType> newGenericType = field->getGenericType();

            // Only substitute if field has generic type that references substitution parameters
            if (newGenericType && referencesSubstitutionParameters(newGenericType, substitutionMap)) {
                newGenericType = newGenericType->substitute(substitutionMap);
                newFieldType = convertGenericTypeToValueType(newGenericType, substitutionMap);
            }

            auto newField = std::make_shared<FieldDefinition>(
                field->getName(),
                newFieldType,
                newGenericType,
                field->getValue(),
                field->isStatic(),
                field->isFinal(),
                field->getAccessModifier() // Preserve access modifier
            );
            target->addInstanceField(fieldName, newField);
        }

        // Copy static fields with AST-based type substitution
        for (const auto& [fieldName, field] : source->getStaticFields())
        {
            value::ValueType newFieldType = field->getType();
            std::shared_ptr<ast::GenericType> newGenericType = field->getGenericType();

            // Only substitute if field has generic type that references substitution parameters
            if (newGenericType && referencesSubstitutionParameters(newGenericType, substitutionMap)) {
                newGenericType = newGenericType->substitute(substitutionMap);
                newFieldType = convertGenericTypeToValueType(newGenericType, substitutionMap);
            }

            auto newField = std::make_shared<FieldDefinition>(
                field->getName(),
                newFieldType,
                newGenericType,
                field->getValue(),
                field->isStatic(),
                field->isFinal(),
                field->getAccessModifier() // Preserve access modifier
            );
            target->addStaticField(fieldName, newField);
        }
    }

    void GenericTypeManager::copyAndSubstituteMethods(
        std::shared_ptr<ClassDefinition> source,
        std::shared_ptr<ClassDefinition> target,
        const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap)
    {
        // Copy instance methods with AST-based type substitution
        for (const auto& [methodName, method] : source->getInstanceMethods())
        {
            auto newMethod = substituteMethodTypes(method, substitutionMap);
            target->addInstanceMethod(methodName, newMethod);
        }

        // Copy static methods with AST-based type substitution
        for (const auto& [methodName, method] : source->getStaticMethods())
        {
            auto newMethod = substituteMethodTypes(method, substitutionMap);
            target->addStaticMethod(methodName, newMethod);
        }
    }

    void GenericTypeManager::copyAndSubstituteConstructors(
        std::shared_ptr<ClassDefinition> source,
        std::shared_ptr<ClassDefinition> target,
        const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap)
    {
        // Copy constructors with AST-based type substitution
        for (const auto& constructor : source->getConstructors())
        {
            auto newConstructor = substituteConstructorTypes(constructor, substitutionMap);
            target->addConstructor(newConstructor);
        }
    }

    bool GenericTypeManager::validateTypeArguments(
        std::shared_ptr<ClassDefinition> genericClass,
        const std::vector<std::string>& typeArguments)
    {
        if (!genericClass) return false;

        // Check that the number of type arguments matches the number of generic parameters
        if (typeArguments.size() != genericClass->getGenericParameterCount()) {
            return false;
        }

        // Check that none of the type arguments are primitive types
        auto& registry = types::getGlobalTypeRegistry();
        for (const auto& typeArg : typeArguments) {
            if (typeArg.empty() || registry.isPrimitiveType(typeArg)) {
                return false;
            }
        }

        return true;
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

    /**
     * @brief Check if a GenericType references any parameters from the substitution map
     * @param genericType The type to check
     * @param substitutionMap The substitution map to check against
     * @return true if the type references any parameter in the map, false otherwise
     */
    bool GenericTypeManager::referencesSubstitutionParameters(
        std::shared_ptr<ast::GenericType> genericType,
        const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap)
    {
        if (!genericType) {
            return false;
        }

        // If this is a generic parameter, check if it's in the substitution map
        if (genericType->isGenericParameter()) {
            std::string paramName = genericType->getGenericName();
            return substitutionMap.find(paramName) != substitutionMap.end();
        }

        // For concrete types, check if any type arguments reference substitution parameters
        if (genericType->isParameterized()) {
            for (const auto& typeArg : genericType->getTypeArguments()) {
                if (referencesSubstitutionParameters(typeArg, substitutionMap)) {
                    return true;
                }
            }
        }

        return false;
    }

    value::ValueType GenericTypeManager::convertGenericTypeToValueType(
        std::shared_ptr<ast::GenericType> genericType,
        const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap)
    {
        if (!genericType) {
            return value::ValueType::OBJECT;
        }

        try {
            // Apply substitution using AST-based method
            auto substitutedType = genericType->substitute(substitutionMap);

            // Convert the substituted GenericType to ValueType
            if (substitutedType->isGenericParameter()) {
                // Still a generic parameter after substitution - return OBJECT
                return value::ValueType::OBJECT;
            }

            // Get the concrete type
            return substitutedType->getConcreteType();

        } catch (const types::TypeConversionException&) {
            // Fallback to OBJECT for conversion errors
            return value::ValueType::OBJECT;
        } catch (...) {
            // Handle unexpected errors gracefully
            return value::ValueType::OBJECT;
        }
    }

    std::shared_ptr<runtimeTypes::klass::MethodDefinition> GenericTypeManager::substituteMethodTypes(
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> originalMethod,
        const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap)
    {
        // Substitute return type using AST-based substitution
        value::ValueType newReturnType = originalMethod->getReturnType();
        std::shared_ptr<ast::GenericType> newGenericReturnType = originalMethod->getGenericReturnType();

        // Only substitute return type if it actually references parameters from the substitution map
        if (newGenericReturnType && referencesSubstitutionParameters(newGenericReturnType, substitutionMap)) {
            newGenericReturnType = newGenericReturnType->substitute(substitutionMap);
            newReturnType = convertGenericTypeToValueType(newGenericReturnType, substitutionMap);
        }

        // Substitute parameter types using AST-based substitution
        auto originalParams = originalMethod->getParameters();
        std::vector<std::pair<std::string, value::ParameterType>> newParams;
        std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>> newGenericParams;
        newParams.reserve(originalParams.size());
        newGenericParams.reserve(originalParams.size());

        const auto& originalGenericParams = originalMethod->getGenericParameters();

        for (size_t i = 0; i < originalParams.size(); ++i) {
            const auto& [paramName, paramType] = originalParams[i];

            // Get corresponding generic type if available
            std::shared_ptr<ast::GenericType> genericParamType;
            if (i < originalGenericParams.size()) {
                genericParamType = originalGenericParams[i].second;
            }

            // Only substitute if the generic type references parameters from the substitution map
            if (genericParamType && referencesSubstitutionParameters(genericParamType, substitutionMap)) {
                auto substitutedGenericType = genericParamType->substitute(substitutionMap);
                value::ValueType newBasicType = convertGenericTypeToValueType(substitutedGenericType, substitutionMap);

                // Create new ParameterType with substituted basic type
                value::ParameterType newParamType(newBasicType);
                if (paramType.isInterface()) {
                    newParamType = value::ParameterType::forInterface(paramType.getInterfaceName());
                } else if (paramType.isClass()) {
                    newParamType = value::ParameterType::forClass(paramType.getClassName());
                }

                newParams.emplace_back(paramName, newParamType);
                newGenericParams.emplace_back(paramName, substitutedGenericType);
            } else {
                // No generic type info or doesn't reference substitution parameters - preserve original
                newParams.emplace_back(paramName, paramType);
                // Preserve original generic type if it exists
                if (genericParamType) {
                    newGenericParams.emplace_back(paramName, genericParamType);
                }
            }
        }

        // Convert string-based substitution map to AST map for storage (empty for now)
        std::unordered_map<std::string, std::string> legacySubstitutionMap;
        for (const auto& [key, genType] : substitutionMap) {
            if (genType && !genType->isGenericParameter()) {
                legacySubstitutionMap[key] = genType->toString();
            }
        }

        // Create new method definition with substituted types
        return std::make_shared<runtimeTypes::klass::MethodDefinition>(
            originalMethod->getName(),
            newReturnType,
            newParams,
            std::vector<std::pair<std::string, value::Value>>{}, // empty arguments
            originalMethod->getBodyPtr(),
            originalMethod->isStatic(),
            newGenericReturnType, // Substituted generic return type
            newGenericParams, // Substituted generic parameters
            originalMethod->getGenericTypeParameters(), // Preserve generic type parameter declarations
            legacySubstitutionMap, // Store legacy substitution map for backward compatibility
            originalMethod->getAccessModifier() // Preserve access modifier
        );
    }

    std::shared_ptr<runtimeTypes::klass::ConstructorDefinition> GenericTypeManager::substituteConstructorTypes(
        std::shared_ptr<runtimeTypes::klass::ConstructorDefinition> originalConstructor,
        const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap)
    {
        // Get the original parameters and substitute their types using AST-based substitution
        auto originalParams = originalConstructor->getParametersWithTypes();
        std::vector<std::pair<std::string, value::ParameterType>> newParams;
        std::vector<std::pair<std::string, std::shared_ptr<ast::GenericType>>> newGenericParams;
        newParams.reserve(originalParams.size());
        newGenericParams.reserve(originalParams.size());

        const auto& originalGenericParams = originalConstructor->getGenericParameters();

        for (size_t i = 0; i < originalParams.size(); ++i) {
            const auto& [paramName, paramType] = originalParams[i];

            // Get corresponding generic type if available
            std::shared_ptr<ast::GenericType> genericParamType;
            if (i < originalGenericParams.size()) {
                genericParamType = originalGenericParams[i].second;
            }

            // Only substitute if the generic type references parameters from the substitution map
            if (genericParamType && referencesSubstitutionParameters(genericParamType, substitutionMap)) {
                auto substitutedGenericType = genericParamType->substitute(substitutionMap);
                value::ValueType newBasicType = convertGenericTypeToValueType(substitutedGenericType, substitutionMap);

                // Create new ParameterType preserving class/interface information
                value::ParameterType newParamType(newBasicType);
                if (paramType.isInterface()) {
                    newParamType = value::ParameterType::forInterface(paramType.getInterfaceName());
                } else if (paramType.isClass()) {
                    newParamType = value::ParameterType::forClass(paramType.getClassName());
                }

                newParams.emplace_back(paramName, newParamType);
                newGenericParams.emplace_back(paramName, substitutedGenericType);
            } else {
                // No generic type info or doesn't reference substitution parameters - preserve original
                newParams.emplace_back(paramName, paramType);
                // Preserve original generic type if it exists
                if (genericParamType) {
                    newGenericParams.emplace_back(paramName, genericParamType);
                }
            }
        }

        // Create new constructor definition with substituted parameter types
        return std::make_shared<runtimeTypes::klass::ConstructorDefinition>(
            newParams,
            originalConstructor->getBodyPtr(),
            newGenericParams,
            originalConstructor->getAccessModifier() // Preserve access modifier
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

        // Create AST-based substitution map
        auto substitutionMap = createASTSubstitutionMap(
            genericParams,
            typeArguments
        );

        // Create specialized method with AST-based type substitution
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

    // AST-based type substitution (replaces naive string substitution)
    std::shared_ptr<ast::GenericType> GenericTypeManager::substituteTypeParameters(
        std::shared_ptr<ast::GenericType> genericType,
        const std::unordered_map<std::string, std::shared_ptr<ast::GenericType>>& substitutionMap)
    {
        if (!genericType) {
            return nullptr;
        }

        // Use the GenericType's built-in substitute method for proper AST handling
        return genericType->substitute(substitutionMap);
    }

    std::unordered_map<std::string, std::shared_ptr<ast::GenericType>> GenericTypeManager::createASTSubstitutionMap(
        const std::vector<GenericTypeParameter>& genericParameters,
        const std::vector<std::string>& typeArguments)
    {
        std::unordered_map<std::string, std::shared_ptr<ast::GenericType>> substitutionMap;

        if (genericParameters.size() != typeArguments.size()) {
            // Invalid argument count - return empty map
            return substitutionMap;
        }

        for (size_t i = 0; i < genericParameters.size(); ++i) {
            const std::string& parameterName = genericParameters[i].name;
            const std::string& typeArgument = typeArguments[i];

            // Parse type argument string to proper GenericType (handles nested generics)
            auto concreteType = parseTypeArgumentToGenericType(typeArgument);

            substitutionMap[parameterName] = concreteType;
        }

        return substitutionMap;
    }

    std::shared_ptr<ast::GenericType> GenericTypeManager::parseTypeArgumentToGenericType(const std::string& typeArgument)
    {
        auto& typeRegistry = types::getGlobalTypeRegistry();

        // Check if it's a simple registered type (class name like "String", "Int")
        if (typeRegistry.hasType(typeArgument)) {
            // Get the ValueType for this registered type
            value::ValueType valueType = typeRegistry.getValueType(typeArgument);
            return std::make_shared<ast::GenericType>(valueType);
        }

        // Check if it's a nested generic instantiation like "List<String>" or "Map<String, Int>"
        if (isGenericInstantiation(typeArgument)) {
            auto [baseName, nestedTypeArgs] = parseGenericInstantiation(typeArgument);

            // Get the base type's ValueType
            value::ValueType baseValueType = value::ValueType::OBJECT;
            if (typeRegistry.hasType(baseName)) {
                baseValueType = typeRegistry.getValueType(baseName);
            }

            // Recursively parse nested type arguments
            std::vector<std::shared_ptr<ast::GenericType>> nestedGenericTypes;
            nestedGenericTypes.reserve(nestedTypeArgs.size());

            for (const auto& nestedArg : nestedTypeArgs) {
                nestedGenericTypes.push_back(parseTypeArgumentToGenericType(nestedArg));
            }

            // Create parameterized GenericType with nested types
            return std::make_shared<ast::GenericType>(baseValueType, nestedGenericTypes);
        }

        // Otherwise treat as a generic parameter name (e.g., "T", "E", "K", "V")
        // This handles cases where a generic parameter is substituted with another generic parameter
        return std::make_shared<ast::GenericType>(typeArgument);
    }

    bool GenericTypeManager::isGenericTypeParameter(const std::string& name)
    {
        if (name.empty()) return false;

        // Single uppercase letter is definitely a generic type parameter
        if (name.length() == 1 && std::isupper(name[0]))
        {
            return true;
        }

        // Uppercase name that's likely a generic parameter (heuristic)
        if (std::isupper(name[0]) && name.length() <= 10)
        {
            bool allLetters = std::all_of(name.begin(), name.end(), ::isalpha);
            if (allLetters)
            {
                return true;
            }
        }

        return false;
    }

    bool GenericTypeManager::hasUnresolvedGenericParams(const std::string& className)
    {
        // Direct generic parameter
        if (isGenericTypeParameter(className))
        {
            return true;
        }

        // Array type containing generic parameters (T[], Element[], etc.)
        if (className.find("[]") != std::string::npos)
        {
            std::string baseType = className.substr(0, className.find("[]"));
            if (isGenericTypeParameter(baseType))
            {
                return true;
            }
        }

        // Generic types with angle brackets
        if (className.find('<') != std::string::npos)
        {
            // Extract type parameters from angle brackets
            size_t start = className.find('<');
            size_t end = className.rfind('>');

            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                std::string typeParams = className.substr(start + 1, end - start - 1);

                // Parse comma-separated type parameters
                std::stringstream ss(typeParams);
                std::string param;

                while (std::getline(ss, param, ','))
                {
                    // Trim whitespace
                    param.erase(0, param.find_first_not_of(" \t"));
                    param.erase(param.find_last_not_of(" \t") + 1);

                    if (isGenericTypeParameter(param))
                    {
                        return true;
                    }
                }
            }

            // Check if this is a concrete generic instantiation
            if (isGenericInstantiation(className))
            {
                auto [baseName, typeArguments] = parseGenericInstantiation(className);
                // Note: We can't check env->findClass here as this is a static utility
                // The caller should handle class existence validation separately
                return true; // Assume it needs special handling
            }
        }

        return false;
    }

    bool GenericTypeManager::validateGenericConstraints(
        const std::vector<GenericTypeParameter>& genericParameters,
        const std::vector<std::string>& typeArguments,
        std::shared_ptr<environment::registry::ClassRegistry> classRegistry,
        std::shared_ptr<runtimeTypes::klass::InterfaceRegistry> interfaceRegistry)
    {
        // Check if argument count matches parameter count
        if (genericParameters.size() != typeArguments.size()) {
            return false;
        }

        // If no registries provided, we can't validate interface constraints
        if (!classRegistry || !interfaceRegistry) {
            // Only check count, assume constraints are satisfied
            return true;
        }

        // Validate each type argument against its corresponding parameter's constraint
        for (size_t i = 0; i < genericParameters.size(); ++i) {
            const auto& param = genericParameters[i];
            const auto& typeArg = typeArguments[i];

            // If this parameter has constraints, validate them
            if (param.hasConstraints()) {
                // Each parameter can only have ONE constraint (validated during parsing)
                const std::string& requiredInterface = param.constraints[0];

                // Check if the type argument implements the required interface
                if (!classImplementsInterface(typeArg, requiredInterface, classRegistry, interfaceRegistry)) {
                    return false;
                }
            }
        }

        return true;
    }

    bool GenericTypeManager::classImplementsInterface(
        const std::string& className,
        const std::string& interfaceName,
        std::shared_ptr<environment::registry::ClassRegistry> classRegistry,
        std::shared_ptr<runtimeTypes::klass::InterfaceRegistry> interfaceRegistry)
    {
        // Validate inputs
        if (className.empty() || interfaceName.empty()) {
            return false;
        }

        // If no registries provided, we can't validate
        if (!classRegistry || !interfaceRegistry) {
            return false;
        }

        try {
            // Extract base class name (handle generic instantiations like "MyClass<int>")
            std::string baseClassName = className;
            size_t anglePos = className.find('<');
            if (anglePos != std::string::npos) {
                baseClassName = className.substr(0, anglePos);
            }

            // Look up the class definition
            auto classDef = classRegistry->findClass(baseClassName);
            if (!classDef) {
                // Class not found - cannot validate
                return false;
            }

            // Extract base interface name (handle generic interfaces like "Comparable<T>")
            std::string baseInterfaceName = interfaceName;
            anglePos = interfaceName.find('<');
            if (anglePos != std::string::npos) {
                baseInterfaceName = interfaceName.substr(0, anglePos);
            }

            // Check if the class implements the interface
            // Use the full interface checking with transitive inheritance support
            return classDef->implementsInterface(baseInterfaceName, interfaceRegistry);

        } catch (...) {
            // Any exception - fail safely
            return false;
        }
    }

}
