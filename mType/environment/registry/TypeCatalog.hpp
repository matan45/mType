#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ClassRegistry.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../types/TypeDescriptors.hpp"
#include "../../types/UnifiedType.hpp"
#include "../../value/ValueType.hpp"

namespace runtimeTypes::klass
{
    class ObjectInstance;
}

namespace environment::registry
{
    /**
     * Unified type catalog. Single Environment-owned home for everything
     * the compiler and VM need to ask about a type:
     *   - Class lifecycle (inherited from ClassRegistry)
     *   - Inheritance graph (inherited from ClassRegistry via InheritanceTracker)
     *   - Primitive / type-name metadata + Box-class mapping
     *   - Generic type parameter lists, collections, arrays
     *   - Subtype / inheritance-distance queries with caching
     *   - Reified generic-type interning + object->type registrations
     *     (inherited from ClassRegistry via ReifiedTypeRegistry)
     *
     * Inheriting ClassRegistry preserves backward compatibility with the
     * many `env->getClassRegistry()->X()` call sites — TypeCatalog IS-A
     * ClassRegistry by Liskov substitution.
     */
    class TypeCatalog : public ClassRegistry
    {
    public:
        TypeCatalog();
        ~TypeCatalog() override = default;

        TypeCatalog(const TypeCatalog&) = delete;
        TypeCatalog& operator=(const TypeCatalog&) = delete;

        std::string getComponentName() const override;

        // ---- Inheritance overrides (maintain subtype cache) ----
        void registerInheritance(const std::string& child, const std::string& parent) override;
        void clearScriptDefinitions() override;

        // ---- Primitive / type-name metadata ----
        void registerPrimitiveType(const std::string& name, value::ValueType vt);
        void registerCustomType(const std::string& name, const std::string& fullyQualifiedName = "");
        bool isPrimitiveType(const std::string& name) const;
        std::string getBoxClassName(const std::string& primitiveName) const;
        bool shouldMapToBoxClass(const std::string& name) const;
        bool hasType(const std::string& name) const;
        types::ExtendedTypeInfo resolveType(const std::string& name) const;
        types::ExtendedTypeInfo parseComplexType(const std::string& s) const;
        value::ValueType getValueType(const std::string& name) const;
        std::vector<std::string> getAllTypeNames() const;

        // ---- Subtype queries (string-keyed, with cache) ----
        [[nodiscard]] bool isSubtypeOf(const std::string& child, const std::string& parent) const;
        [[nodiscard]] int getInheritanceDistance(const std::string& child,
                                                 const std::string& parent) const;

        // ---- Generics ----
        void registerGenericType(const std::string& name,
                                 const std::vector<std::string>& params);
        bool isGenericType(const std::string& name) const;
        std::vector<std::string> getGenericTypeParameters(const std::string& name) const;
        static bool isGenericParameter(const std::string& name);

        // ---- Collections / arrays ----
        void registerCollectionType(const std::string& name,
                                    const std::vector<std::string>& params);
        bool isCollectionType(const std::string& name) const;
        void registerArrayType(const std::string& element, int dimensions);
        bool isArrayType(const std::string& name) const;
        static std::string createArrayTypeName(const std::string& element, int dimensions);
        static std::pair<std::string, int>
            parseArrayTypeName(const std::string& arrayTypeName);

        // ---- Reified types (forwarded to inherited ReifiedTypeRegistry) ----
        types::UnifiedTypePtr intern(const types::UnifiedTypePtr& t);
        bool isInterned(const types::UnifiedTypePtr& t) const;
        types::UnifiedTypePtr findByCanonicalString(const std::string& s) const;
        // getReifiedClassType is inherited from ClassRegistry.
        // isSameReifiedType is inherited from ClassRegistry.
        bool isInstance(const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj,
                        const types::UnifiedTypePtr& t) const;
        types::UnifiedTypePtr getReifiedType(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) const;
        void registerObjectType(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj,
            const types::UnifiedTypePtr& t);
        void unregisterObject(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj);
        std::vector<types::UnifiedTypePtr> getTypeArguments(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) const;
        bool haveSameReifiedType(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& a,
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& b) const;
        std::vector<types::UnifiedTypePtr> getAllInternedTypes() const;
        size_t cleanupOrphanedRegistrations();

    private:
        static constexpr int MAX_INHERITANCE_DEPTH = 20;

        // Type-system data (absorbed from former types::TypeRegistry)
        std::unordered_map<std::string, types::ExtendedTypeInfo> typeMap;
        std::unordered_map<std::string, value::ValueType> primitiveTypes;
        std::unordered_map<std::string, types::ExtendedTypeInfo> arrayTypeCache;
        std::unordered_map<std::string, std::vector<std::string>> genericTypeParameters;
        std::unordered_set<std::string> knownGenericTypes;
        std::unordered_set<std::string> collectionTypes;
        std::unordered_map<std::string, std::string> primitiveToBoxMapping;

        // Subtype query cache (string-keyed)
        mutable std::unordered_map<std::string, bool> subtypeCache;

        bool validateTypeArguments(const std::string& genericType,
                                   const std::vector<std::string>& typeArgs) const;
        void initializePrimitiveTypes();
    };
}
