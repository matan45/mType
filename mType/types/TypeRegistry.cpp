#include "TypeRegistry.hpp"
#include "StringUtils.hpp"
#include <regex>
#include <sstream>
#include <algorithm>
#include <cctype>


namespace types {

    // ArrayTypeParser implementation
    std::string ArrayTypeParser::createArrayTypeName(const std::string& elementType, int dimensions) {
        std::string result = elementType;
        for (int i = 0; i < dimensions; ++i) {
            result += "[]";
        }
        return result;
    }

    std::pair<std::string, int> ArrayTypeParser::parseArrayTypeName(const std::string& arrayTypeName) {
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

    // GenericInstantiationParser implementation
    std::pair<std::string, std::vector<std::string>> GenericInstantiationParser::parse(const std::string& typeName) {
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
                std::string trimmed = StringUtils::trimWhitespace(currentArg);
                if (!trimmed.empty()) {
                    typeArgs.push_back(trimmed);
                }
                currentArg.clear();
            } else {
                currentArg += c;
            }
        }

        // Add the last argument
        std::string trimmed = StringUtils::trimWhitespace(currentArg);
        if (!trimmed.empty()) {
            typeArgs.push_back(trimmed);
        }

        return {baseName, typeArgs};
    }

    // InheritanceChainTraverser implementation
    InheritanceChainTraverser::InheritanceChainTraverser(const std::unordered_map<std::string, std::string>& inheritance)
        : classInheritance(inheritance) {}

    bool InheritanceChainTraverser::traverse(const std::string& childType, const std::string& targetParent) const {
        std::string current = childType;
        std::unordered_set<std::string> visited;
        int depth = 0;

        while (depth < MAX_DEPTH) {
            auto inheritanceIt = classInheritance.find(current);
            if (inheritanceIt == classInheritance.end()) {
                return false; // No parent found
            }

            const std::string& parent = inheritanceIt->second;

            // Cycle detection
            if (visited.find(parent) != visited.end()) {
                return false;
            }
            visited.insert(parent);

            // Check if we've found the target parent
            if (parent == targetParent) {
                return true;
            }

            // Continue up the inheritance chain
            current = parent;
            depth++;
        }

        return false; // Depth limit reached
    }

    // ExtendedTypeInfo constructors
    ExtendedTypeInfo::ExtendedTypeInfo()
        : baseType(value::ValueType::VOID), isArray(false), arrayDimensions(0), isGeneric(false) {}

    ExtendedTypeInfo::ExtendedTypeInfo(value::ValueType type, const std::string& name)
        : baseType(type), typeName(name), isArray(false), arrayDimensions(0), isGeneric(false) {}

    ExtendedTypeInfo::ExtendedTypeInfo(value::ValueType type, const std::string& name, int dimensions)
        : baseType(type), typeName(name), isArray(dimensions > 0), arrayDimensions(dimensions), isGeneric(false) {}

    // TypeRegistry implementation
    TypeRegistry::TypeRegistry() {
        initializePrimitiveTypes();
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

        std::string arrayTypeName = ArrayTypeParser::createArrayTypeName(elementType, dimensions);

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
        auto [baseName, typeArgs] = GenericInstantiationParser::parse(typeName);
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
        auto [baseName, typeArgs] = GenericInstantiationParser::parse(typeName);
        return knownGenericTypes.find(baseName) != knownGenericTypes.end();
    }

    bool TypeRegistry::isCollectionType(const std::string& typeName) const {
        auto [baseName, typeArgs] = GenericInstantiationParser::parse(typeName);
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
            auto [elementType, dimensions] = ArrayTypeParser::parseArrayTypeName(typeName);

            // Ensure element type exists
            if (!hasType(elementType)) {
                throw errors::TypeResolutionException("Unknown array element type: " + elementType);
            }

            ExtendedTypeInfo info(value::ValueType::ARRAY, typeName, dimensions);
            info.fullyQualifiedName = typeName;
            return info;
        }

        // Generic type instantiation
        auto [baseName, typeArgs] = GenericInstantiationParser::parse(typeName);
        if (knownGenericTypes.find(baseName) != knownGenericTypes.end()) {
            if (!validateTypeArguments(baseName, typeArgs)) {
                throw errors::TypeResolutionException("Invalid type arguments for generic type: " + typeName);
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

        throw errors::TypeResolutionException("Unknown type: " + typeName);
    }

    value::ValueType TypeRegistry::getValueType(const std::string& typeName) const {
        try {
            ExtendedTypeInfo info = resolveType(typeName);
            return info.baseType;
        } catch (const errors::TypeResolutionException&) {
            // For backward compatibility, return OBJECT for unknown types
            return value::ValueType::OBJECT;
        }
    }

    ExtendedTypeInfo TypeRegistry::parseComplexType(const std::string& typeString) const {
        // Handle nested generics like Array<List<String>>
        // Remove all whitespace for parsing
        std::string trimmed = typeString;
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
        return ArrayTypeParser::createArrayTypeName(elementType, dimensions);
    }

    std::pair<std::string, int> TypeRegistry::parseArrayTypeName(const std::string& arrayTypeName) {
        return ArrayTypeParser::parseArrayTypeName(arrayTypeName);
    }

    bool TypeRegistry::isGenericParameter(const std::string& typeName) {
        // Generic type parameters are single uppercase letters (T, K, V, E, etc.)
        if (typeName.empty()) return false;

        // Single letter, uppercase
        return typeName.length() == 1 && std::isupper(typeName[0]);
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

    bool TypeRegistry::isPrimitiveType(const std::string& typeName) const {
        return primitiveTypes.find(typeName) != primitiveTypes.end();
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

            // Reject primitive types as generic type arguments
            if (isPrimitiveType(typeArg)) {
                return false;
            }

            // Type arguments must be object types (classes, interfaces, or other generic types)
            // For now, accept any non-empty, non-primitive string
        }

        return true;
    }

    void TypeRegistry::registerInheritance(const std::string& childType, const std::string& parentType) {
        classInheritance[childType] = parentType;
        // Clear cache when inheritance relationships change
        subtypeCache.clear();
    }

    bool TypeRegistry::isSubtypeOf(const std::string& childType, const std::string& parentType) const {
        // Exact match
        if (childType == parentType) {
            return true;
        }

        // Check cache
        std::string cacheKey = childType + "->" + parentType;
        auto it = subtypeCache.find(cacheKey);
        if (it != subtypeCache.end()) {
            return it->second;
        }

        // Traverse inheritance chain using helper
        InheritanceChainTraverser traverser(classInheritance);
        bool result = traverser.traverse(childType, parentType);

        // Cache the result
        subtypeCache[cacheKey] = result;
        return result;
    }

    // Global registry instance
    TypeRegistry& getGlobalTypeRegistry() {
        static TypeRegistry instance;
        return instance;
    }

} // namespace types