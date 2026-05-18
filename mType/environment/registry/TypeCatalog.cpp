#include "TypeCatalog.hpp"

#include "../../errors/TypeResolutionException.hpp"
#include "../../types/ReifiedTypeRegistry.hpp"

#include <algorithm>
#include <cctype>

namespace environment::registry
{
    TypeCatalog::TypeCatalog()
    {
        initializePrimitiveTypes();
    }

    std::string TypeCatalog::getComponentName() const
    {
        return "TypeCatalog";
    }

    // ---- Inheritance overrides ----

    void TypeCatalog::registerInheritance(const std::string& child, const std::string& parent)
    {
        ClassRegistry::registerInheritance(child, parent);
        // Inheritance edge changed — string-keyed subtype cache must reset.
        subtypeCache.clear();
    }

    void TypeCatalog::clearScriptDefinitions()
    {
        ClassRegistry::clearScriptDefinitions();
        typeMap.clear();
        primitiveTypes.clear();
        arrayTypeCache.clear();
        genericTypeParameters.clear();
        knownGenericTypes.clear();
        collectionTypes.clear();
        primitiveToBoxMapping.clear();
        subtypeCache.clear();
        initializePrimitiveTypes();
    }

    // ---- Primitive / type-name metadata ----

    void TypeCatalog::registerPrimitiveType(const std::string& name, value::ValueType vt)
    {
        primitiveTypes[name] = vt;
        typeMap[name] = types::ExtendedTypeInfo(vt, name);
    }

    void TypeCatalog::registerCustomType(const std::string& name, const std::string& fullyQualifiedName)
    {
        types::ExtendedTypeInfo info(value::ValueType::OBJECT, name);
        info.fullyQualifiedName = fullyQualifiedName.empty() ? name : fullyQualifiedName;
        typeMap[name] = info;
    }

    bool TypeCatalog::isPrimitiveType(const std::string& name) const
    {
        return primitiveTypes.find(name) != primitiveTypes.end();
    }

    std::string TypeCatalog::getBoxClassName(const std::string& primitiveName) const
    {
        auto it = primitiveToBoxMapping.find(primitiveName);
        return it != primitiveToBoxMapping.end() ? it->second : "";
    }

    bool TypeCatalog::shouldMapToBoxClass(const std::string& name) const
    {
        return primitiveToBoxMapping.find(name) != primitiveToBoxMapping.end();
    }

    bool TypeCatalog::hasType(const std::string& name) const
    {
        if (typeMap.find(name) != typeMap.end())
        {
            return true;
        }

        if (isArrayType(name))
        {
            return true;
        }

        auto [baseName, typeArgs] = types::GenericInstantiationParser::parse(name);
        if (knownGenericTypes.find(baseName) != knownGenericTypes.end())
        {
            return validateTypeArguments(baseName, typeArgs);
        }

        if (isGenericParameter(name))
        {
            return true;
        }

        return false;
    }

    types::ExtendedTypeInfo TypeCatalog::resolveType(const std::string& name) const
    {
        auto it = typeMap.find(name);
        if (it != typeMap.end())
        {
            return it->second;
        }

        if (isArrayType(name))
        {
            auto [elementType, dimensions] = types::ArrayTypeParser::parseArrayTypeName(name);
            if (!hasType(elementType))
            {
                throw errors::TypeResolutionException("Unknown array element type: " + elementType);
            }
            types::ExtendedTypeInfo info(value::ValueType::ARRAY, name, dimensions);
            info.fullyQualifiedName = name;
            return info;
        }

        auto [baseName, typeArgs] = types::GenericInstantiationParser::parse(name);
        if (knownGenericTypes.find(baseName) != knownGenericTypes.end())
        {
            if (!validateTypeArguments(baseName, typeArgs))
            {
                throw errors::TypeResolutionException("Invalid type arguments for generic type: " + name);
            }
            types::ExtendedTypeInfo info(value::ValueType::OBJECT, name);
            info.isGeneric = true;
            info.genericParameters = typeArgs;
            info.fullyQualifiedName = name;
            return info;
        }

        if (isGenericParameter(name))
        {
            types::ExtendedTypeInfo info(value::ValueType::OBJECT, name);
            info.fullyQualifiedName = name;
            return info;
        }

        throw errors::TypeResolutionException("Unknown type: " + name);
    }

    types::ExtendedTypeInfo TypeCatalog::parseComplexType(const std::string& s) const
    {
        std::string trimmed = s;
        trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
        return resolveType(trimmed);
    }

    value::ValueType TypeCatalog::getValueType(const std::string& name) const
    {
        try
        {
            return resolveType(name).baseType;
        }
        catch (const errors::TypeResolutionException&)
        {
            return value::ValueType::OBJECT;
        }
    }

    std::vector<std::string> TypeCatalog::getAllTypeNames() const
    {
        std::vector<std::string> names;
        names.reserve(typeMap.size());
        for (const auto& [name, info] : typeMap)
        {
            names.push_back(name);
        }
        return names;
    }

    // ---- Subtype queries ----

    bool TypeCatalog::isSubtypeOf(const std::string& child, const std::string& parent) const
    {
        if (child == parent)
        {
            return true;
        }

        std::string key = child + "->" + parent;
        auto cacheIt = subtypeCache.find(key);
        if (cacheIt != subtypeCache.end())
        {
            return cacheIt->second;
        }

        // Walk parent chain via inherited InheritanceTracker (string-keyed lookup).
        std::string current = child;
        std::unordered_set<std::string> visited;
        int depth = 0;
        bool result = false;

        while (depth < MAX_INHERITANCE_DEPTH)
        {
            std::string nextParent = getParentClass(current);
            if (nextParent.empty())
            {
                break;
            }
            if (visited.find(nextParent) != visited.end())
            {
                break; // cycle
            }
            visited.insert(nextParent);
            if (nextParent == parent)
            {
                result = true;
                break;
            }
            current = nextParent;
            depth++;
        }

        subtypeCache[key] = result;
        return result;
    }

    int TypeCatalog::getInheritanceDistance(const std::string& child,
                                            const std::string& parent) const
    {
        if (child == parent)
        {
            return 0;
        }

        std::string current = child;
        std::unordered_set<std::string> visited;
        int distance = 0;

        while (distance < MAX_INHERITANCE_DEPTH)
        {
            std::string nextParent = getParentClass(current);
            if (nextParent.empty())
            {
                return -1;
            }
            if (visited.find(nextParent) != visited.end())
            {
                return -1;
            }
            visited.insert(nextParent);
            distance++;
            if (nextParent == parent)
            {
                return distance;
            }
            current = nextParent;
        }

        return -1;
    }

    // ---- Generics ----

    void TypeCatalog::registerGenericType(const std::string& name,
                                          const std::vector<std::string>& params)
    {
        genericTypeParameters[name] = params;
        knownGenericTypes.insert(name);

        types::ExtendedTypeInfo info(value::ValueType::OBJECT, name);
        info.isGeneric = true;
        info.genericParameters = params;
        typeMap[name] = info;
    }

    bool TypeCatalog::isGenericType(const std::string& name) const
    {
        auto [baseName, typeArgs] = types::GenericInstantiationParser::parse(name);
        return knownGenericTypes.find(baseName) != knownGenericTypes.end();
    }

    std::vector<std::string>
    TypeCatalog::getGenericTypeParameters(const std::string& name) const
    {
        auto it = genericTypeParameters.find(name);
        return it != genericTypeParameters.end() ? it->second : std::vector<std::string>{};
    }

    bool TypeCatalog::isGenericParameter(const std::string& name)
    {
        if (name.empty()) return false;
        return name.length() == 1 && std::isupper(static_cast<unsigned char>(name[0]));
    }

    // ---- Collections / arrays ----

    void TypeCatalog::registerCollectionType(const std::string& name,
                                             const std::vector<std::string>& params)
    {
        registerGenericType(name, params);
        collectionTypes.insert(name);
    }

    bool TypeCatalog::isCollectionType(const std::string& name) const
    {
        auto [baseName, typeArgs] = types::GenericInstantiationParser::parse(name);
        return collectionTypes.find(baseName) != collectionTypes.end();
    }

    void TypeCatalog::registerArrayType(const std::string& element, int dimensions)
    {
        if (dimensions <= 0) return;

        std::string arrayTypeName = types::ArrayTypeParser::createArrayTypeName(element, dimensions);
        types::ExtendedTypeInfo info(value::ValueType::ARRAY, arrayTypeName, dimensions);
        info.fullyQualifiedName = arrayTypeName;
        arrayTypeCache[arrayTypeName] = info;
        typeMap[arrayTypeName] = info;
    }

    bool TypeCatalog::isArrayType(const std::string& name) const
    {
        return name.find("[]") != std::string::npos;
    }

    std::string TypeCatalog::createArrayTypeName(const std::string& element, int dimensions)
    {
        return types::ArrayTypeParser::createArrayTypeName(element, dimensions);
    }

    std::pair<std::string, int>
    TypeCatalog::parseArrayTypeName(const std::string& arrayTypeName)
    {
        return types::ArrayTypeParser::parseArrayTypeName(arrayTypeName);
    }

    // ---- Reified types ----

    types::UnifiedTypePtr TypeCatalog::intern(const types::UnifiedTypePtr& t)
    {
        return getReifiedTypeRegistry()->intern(t);
    }

    bool TypeCatalog::isInterned(const types::UnifiedTypePtr& t) const
    {
        return getReifiedTypeRegistry()->isInterned(t);
    }

    types::UnifiedTypePtr TypeCatalog::findByCanonicalString(const std::string& s) const
    {
        return getReifiedTypeRegistry()->findByCanonicalString(s);
    }

    bool TypeCatalog::isInstance(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj,
        const types::UnifiedTypePtr& t) const
    {
        return getReifiedTypeRegistry()->isInstance(obj, t);
    }

    types::UnifiedTypePtr TypeCatalog::getReifiedType(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) const
    {
        return getReifiedTypeRegistry()->getReifiedType(obj);
    }

    void TypeCatalog::registerObjectType(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj,
        const types::UnifiedTypePtr& t)
    {
        getReifiedTypeRegistry()->registerObjectType(obj, t);
    }

    void TypeCatalog::unregisterObject(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj)
    {
        getReifiedTypeRegistry()->unregisterObject(obj);
    }

    std::vector<types::UnifiedTypePtr> TypeCatalog::getTypeArguments(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) const
    {
        return getReifiedTypeRegistry()->getTypeArguments(obj);
    }

    bool TypeCatalog::haveSameReifiedType(
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& a,
        const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& b) const
    {
        return getReifiedTypeRegistry()->haveSameReifiedType(a, b);
    }

    std::vector<types::UnifiedTypePtr> TypeCatalog::getAllInternedTypes() const
    {
        return getReifiedTypeRegistry()->getAllInternedTypes();
    }

    size_t TypeCatalog::cleanupOrphanedRegistrations()
    {
        return getReifiedTypeRegistry()->cleanupOrphanedRegistrations();
    }

    // ---- Private helpers ----

    bool TypeCatalog::validateTypeArguments(const std::string& genericType,
                                            const std::vector<std::string>& typeArgs) const
    {
        auto it = genericTypeParameters.find(genericType);
        if (it == genericTypeParameters.end())
        {
            return false;
        }
        if (typeArgs.size() != it->second.size())
        {
            return false;
        }
        for (const auto& typeArg : typeArgs)
        {
            if (typeArg.empty())
            {
                return false;
            }
        }
        return true;
    }

    void TypeCatalog::initializePrimitiveTypes()
    {
        // Primitive types — VM-level entries
        registerPrimitiveType("int", value::ValueType::INT);
        registerPrimitiveType("float", value::ValueType::FLOAT);
        registerPrimitiveType("bool", value::ValueType::BOOL);
        registerPrimitiveType("string", value::ValueType::STRING);
        registerPrimitiveType("void", value::ValueType::VOID);
        registerPrimitiveType("null", value::ValueType::NULL_TYPE);
        registerPrimitiveType("lambda", value::ValueType::LAMBDA);

        // Object as the synthetic root class — registered here so scripts can
        // reference it (e.g. `Object x = new Int[3];`, `arr isClassOf Object`)
        // before its ClassDefinition is loaded by the import system.
        registerCustomType("Object", "Object");

        // Box classes (lib/primitives/*.mt) — type-name entries only; the
        // actual ClassDefinitions arrive via the import system.
        registerCustomType("Int", "Int");
        registerCustomType("Float", "Float");
        registerCustomType("Bool", "Bool");
        registerCustomType("String", "String");

        // Lowercase primitive → Box class mapping for Pure OOP.
        primitiveToBoxMapping["int"] = "Int";
        primitiveToBoxMapping["float"] = "Float";
        primitiveToBoxMapping["bool"] = "Bool";
        primitiveToBoxMapping["string"] = "String";

        // Box class inheritance edges (value classes have no `extends` syntax).
        // Call the inherited ClassRegistry version directly so we don't recurse
        // through TypeCatalog::registerInheritance (we already control cache state).
        ClassRegistry::registerInheritance("Int", "Object");
        ClassRegistry::registerInheritance("Float", "Object");
        ClassRegistry::registerInheritance("Bool", "Object");
        ClassRegistry::registerInheritance("String", "Object");
    }
}
