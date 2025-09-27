#include "TypeRegistry.hpp"
#include <regex>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace types {

    TypeRegistry::TypeRegistry() {
        initializePrimitiveTypes();
        initializeCollectionTypes();
    }

    void TypeRegistry::registerPrimitiveType(const std::string& name, value::ValueType valueType) {
        primitiveTypes[name] = valueType;
        typeMap[name] = ExtendedTypeInfo(valueType, name);
    }

    void TypeRegistry::registerCustomType(const std::string& name, const std::string& fullyQualifiedName) {
        ExtendedTypeInfo info(value::ValueType::OBJECT, name);
        info.fullyQualifiedName = fullyQualifiedName.empty() ? name : fullyQualifiedName;
        typeMap[name] = info;
    }

    void TypeRegistry::registerGenericType(const std::string& name, const std::vector<std::string>& typeParameters) {
        genericTypeParameters[name] = typeParameters;
        knownGenericTypes.insert(name);

        ExtendedTypeInfo info(value::ValueType::OBJECT, name);
        info.isGeneric = true;
        info.genericParameters = typeParameters;
        typeMap[name] = info;
    }

    void TypeRegistry::registerCollectionType(const std::string& name, const std::vector<std::string>& typeParameters) {
        registerGenericType(name, typeParameters);
        collectionTypes.insert(name);
    }

    void TypeRegistry::registerArrayType(const std::string& elementType, int dimensions) {
        if (dimensions <= 0) return;

        std::string arrayTypeName = createArrayTypeName(elementType, dimensions);

        ExtendedTypeInfo info(value::ValueType::ARRAY, arrayTypeName, dimensions);
        info.fullyQualifiedName = arrayTypeName;

        arrayTypeCache[arrayTypeName] = info;
        typeMap[arrayTypeName] = info;
    }

    bool TypeRegistry::hasType(const std::string& typeName) const {
        // Check direct type map
        if (typeMap.find(typeName) != typeMap.end()) {
            return true;
        }

        // Check if it's an array type
        if (isArrayType(typeName)) {
            return true;
        }

        // Check if it's a generic instantiation
        auto [baseName, typeArgs] = parseGenericInstantiation(typeName);
        if (knownGenericTypes.find(baseName) != knownGenericTypes.end()) {
            return validateTypeArguments(baseName, typeArgs);
        }

        // Check if it's a generic type parameter
        if (isGenericParameter(typeName)) {
            return true;
        }

        return false;
    }

    bool TypeRegistry::isArrayType(const std::string& typeName) const {
        return typeName.find("[]") != std::string::npos;
    }

    bool TypeRegistry::isGenericType(const std::string& typeName) const {
        auto [baseName, typeArgs] = parseGenericInstantiation(typeName);
        return knownGenericTypes.find(baseName) != knownGenericTypes.end();
    }

    bool TypeRegistry::isCollectionType(const std::string& typeName) const {
        auto [baseName, typeArgs] = parseGenericInstantiation(typeName);
        return collectionTypes.find(baseName) != collectionTypes.end();
    }

    ExtendedTypeInfo TypeRegistry::resolveType(const std::string& typeName) const {
        // Direct lookup first
        auto it = typeMap.find(typeName);
        if (it != typeMap.end()) {
            return it->second;
        }

        // Array type handling
        if (isArrayType(typeName)) {
            auto [elementType, dimensions] = parseArrayTypeName(typeName);

            // Ensure element type exists
            if (!hasType(elementType)) {
                throw TypeResolutionException("Unknown array element type: " + elementType);
            }

            ExtendedTypeInfo info(value::ValueType::ARRAY, typeName, dimensions);
            info.fullyQualifiedName = typeName;
            return info;
        }

        // Generic type instantiation
        auto [baseName, typeArgs] = parseGenericInstantiation(typeName);
        if (knownGenericTypes.find(baseName) != knownGenericTypes.end()) {
            if (!validateTypeArguments(baseName, typeArgs)) {
                throw TypeResolutionException("Invalid type arguments for generic type: " + typeName);
            }

            ExtendedTypeInfo info(value::ValueType::OBJECT, typeName);
            info.isGeneric = true;
            info.genericParameters = typeArgs;
            info.fullyQualifiedName = typeName;
            return info;
        }

        // Generic type parameter
        if (isGenericParameter(typeName)) {
            ExtendedTypeInfo info(value::ValueType::OBJECT, typeName);
            info.fullyQualifiedName = typeName;
            return info;
        }

        throw TypeResolutionException("Unknown type: " + typeName);
    }

    value::ValueType TypeRegistry::getValueType(const std::string& typeName) const {
        try {
            ExtendedTypeInfo info = resolveType(typeName);
            return info.baseType;
        } catch (const TypeResolutionException&) {
            // For backward compatibility, return OBJECT for unknown types
            return value::ValueType::OBJECT;
        }
    }

    ExtendedTypeInfo TypeRegistry::parseComplexType(const std::string& typeString) const {
        // Handle nested generics like Array<List<String>>
        std::string trimmed = typeString;

        // Remove whitespace
        trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());

        return resolveType(trimmed);
    }

    std::vector<std::string> TypeRegistry::getGenericTypeParameters(const std::string& typeName) const {
        auto it = genericTypeParameters.find(typeName);
        if (it != genericTypeParameters.end()) {
            return it->second;
        }
        return {};
    }

    std::string TypeRegistry::createArrayTypeName(const std::string& elementType, int dimensions) {
        std::string result = elementType;
        for (int i = 0; i < dimensions; ++i) {
            result += "[]";
        }
        return result;
    }

    std::pair<std::string, int> TypeRegistry::parseArrayTypeName(const std::string& arrayTypeName) {
        std::string elementType = arrayTypeName;
        int dimensions = 0;

        // Count and remove array brackets
        size_t pos = arrayTypeName.length();
        while (pos >= 2 && arrayTypeName.substr(pos - 2, 2) == "[]") {
            dimensions++;
            pos -= 2;
        }

        if (dimensions > 0) {
            elementType = arrayTypeName.substr(0, pos);
        }

        return {elementType, dimensions};
    }

    bool TypeRegistry::isGenericParameter(const std::string& typeName) {
        // Generic type parameters are typically single uppercase letters (T, K, V, E)
        // or short uppercase names (Key, Value, Element)
        if (typeName.empty()) return false;

        // Single letter, uppercase
        if (typeName.length() == 1 && std::isupper(typeName[0])) {
            return true;
        }

        // Common generic parameter patterns
        static const std::unordered_set<std::string> commonParams = {
            "T", "E", "K", "V", "Key", "Value", "Element", "Type"
        };

        return commonParams.find(typeName) != commonParams.end();
    }

    std::vector<std::string> TypeRegistry::getAllTypeNames() const {
        std::vector<std::string> names;
        names.reserve(typeMap.size());

        for (const auto& [name, info] : typeMap) {
            names.push_back(name);
        }

        return names;
    }

    void TypeRegistry::clear() {
        typeMap.clear();
        primitiveTypes.clear();
        arrayTypeCache.clear();
        genericTypeParameters.clear();
        knownGenericTypes.clear();
        collectionTypes.clear();
        typeFactories.clear();
    }

    void TypeRegistry::initializePrimitiveTypes() {
        registerPrimitiveType("int", value::ValueType::INT);
        registerPrimitiveType("float", value::ValueType::FLOAT);
        registerPrimitiveType("bool", value::ValueType::BOOL);
        registerPrimitiveType("string", value::ValueType::STRING);
        registerPrimitiveType("void", value::ValueType::VOID);
        registerPrimitiveType("null", value::ValueType::NULL_TYPE);
        registerPrimitiveType("lambda", value::ValueType::LAMBDA);
    }

    void TypeRegistry::initializeCollectionTypes() {
        // Collection framework types from mType language
        registerCollectionType("Array", {"T"});
        registerCollectionType("List", {"T"});
        registerCollectionType("Set", {"T"});
        registerCollectionType("Map", {"K", "V"});
        registerCollectionType("HashMap", {"K", "V"});
        registerCollectionType("HashSet", {"T"});
        registerCollectionType("Stack", {"T"});
        registerCollectionType("Queue", {"T"});
        registerCollectionType("Vector", {"T"});
        registerCollectionType("Dictionary", {"K", "V"});

        // Utility types
        registerGenericType("Optional", {"T"});
        registerGenericType("Nullable", {"T"});
        registerGenericType("Pair", {"T", "U"});
        registerGenericType("Tuple", {"T", "U"});

        // Pre-register common array types for performance
        std::vector<std::string> commonElementTypes = {"int", "float", "bool", "string"};
        for (const auto& elementType : commonElementTypes) {
            for (int dimensions = 1; dimensions <= 3; ++dimensions) {
                registerArrayType(elementType, dimensions);
            }
        }
    }

    std::pair<std::string, std::vector<std::string>> TypeRegistry::parseGenericInstantiation(const std::string& typeName) const {
        size_t anglePos = typeName.find('<');
        if (anglePos == std::string::npos) {
            return {typeName, {}};
        }

        size_t endPos = typeName.rfind('>');
        if (endPos == std::string::npos || endPos <= anglePos) {
            return {typeName, {}};
        }

        std::string baseName = typeName.substr(0, anglePos);
        std::string typeArgsString = typeName.substr(anglePos + 1, endPos - anglePos - 1);

        // Parse type arguments with nested generic support
        std::vector<std::string> typeArgs;
        std::string currentArg;
        int depth = 0;

        for (char c : typeArgsString) {
            if (c == '<') {
                depth++;
                currentArg += c;
            } else if (c == '>') {
                depth--;
                currentArg += c;
            } else if (c == ',' && depth == 0) {
                // Top-level comma - this separates type arguments
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

        return {baseName, typeArgs};
    }

    bool TypeRegistry::validateTypeArguments(const std::string& genericType, const std::vector<std::string>& typeArgs) const {
        auto paramIt = genericTypeParameters.find(genericType);
        if (paramIt == genericTypeParameters.end()) {
            return false;
        }

        const auto& expectedParams = paramIt->second;
        if (typeArgs.size() != expectedParams.size()) {
            return false;
        }

        // Validate each type argument
        for (const auto& typeArg : typeArgs) {
            if (typeArg.empty()) {
                return false;
            }
            // Type arguments can be primitives, generics, or other valid types
            // For now, accept any non-empty string
        }

        return true;
    }

    // Global registry instance
    TypeRegistry& getGlobalTypeRegistry() {
        static TypeRegistry instance;
        return instance;
    }

} // namespace types