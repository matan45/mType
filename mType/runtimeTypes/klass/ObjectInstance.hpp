#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "../../value/ValueType.hpp"
#include "../../value/PrimitiveTypeTag.hpp"
#include "ClassDefinition.hpp"
#include "FieldDefinition.hpp"

namespace runtimeTypes::klass
{
    using namespace value;
    class ObjectInstance : public std::enable_shared_from_this<ObjectInstance>
    {
    private:
        std::shared_ptr<ClassDefinition> classDefinition;
        std::unordered_map<std::string, Value> fieldValues;

        // Generic type bindings: T -> String, K -> int, etc.
        std::unordered_map<std::string, std::string> genericTypeBindings;

        // NEW: Method dispatch cache for polymorphic method lookup performance
        mutable std::unordered_map<std::string, std::shared_ptr<MethodDefinition>> methodCache;

        // Helper method for field value comparison
        static bool compareFieldValues(const Value& thisValue, const Value& otherValue, int depth = 0);

        // Depth-limited helpers to prevent infinite recursion on circular references
        std::string getContentHashImpl(int depth) const;
        bool contentEqualsImpl(const ObjectInstance& other, int depth) const;

        // GC: Flag to track if this instance is registered with GC
        bool gcRegistered = false;

        // Phase 6 (IC): Vector-based field storage for O(1) indexed access.
        // Marked mutable so ensureFieldVector() can be a const-qualified lazy
        // initializer — the underlying ObjectInstance identity is unchanged,
        // only a cached view is populated. Canonical C++ pattern for lazy
        // cached state; also removes a const_cast at JitHelpers_Core.cpp:20.
        mutable std::vector<Value> fieldVector;
        mutable bool fieldVectorInitialized = false;

        // Fast primitive type tag (avoids string comparisons in hot paths)
        value::PrimitiveTypeTag primitiveTag_ = value::PrimitiveTypeTag::NONE;

    public :
        ObjectInstance(std::shared_ptr<ClassDefinition> classDef)
            : classDefinition(classDef)
        {
            if (classDef) {
                fieldValues.reserve(classDef->getTotalFieldCount());
                primitiveTag_ = value::classNameToPrimitiveTag(classDef->getName());
                // Eagerly initialize field vector for primitive types so
                // unboxInt/unboxFloat avoid ensureFieldVector() on every call
                if (primitiveTag_ != value::PrimitiveTypeTag::NONE) {
                    ensureFieldVector();
                }
            }
        }

        // Constructor with generic type bindings
        ObjectInstance(std::shared_ptr<ClassDefinition> classDef,
                      const std::unordered_map<std::string, std::string>& typeBindings)
            : classDefinition(classDef), genericTypeBindings(typeBindings)
        {
            if (classDef) {
                fieldValues.reserve(classDef->getTotalFieldCount());
                primitiveTag_ = value::classNameToPrimitiveTag(classDef->getName());
                if (primitiveTag_ != value::PrimitiveTypeTag::NONE) {
                    ensureFieldVector();
                }
            }
        }

        // GC: Register this instance with garbage collector
        // Call this after creating a shared_ptr to the instance
        void registerWithGC();

        std::shared_ptr<FieldDefinition> getField(const std::string& fieldName) const;
        Value getFieldValue(const std::string& fieldName) const;
        std::shared_ptr<ClassDefinition> getClassDefinition() const;
        // MYT-200: raw-pointer fast path for IC hot sites. Skips the non-inline
        // getClassDefinition() call and its by-value shared_ptr copy (atomic
        // refcount++/--). Caller MUST NOT outlive the instance — for ownership
        // (GC roots, error context, ValueObject construction) keep using
        // getClassDefinition(). Must stay header-only to inline.
        const ClassDefinition* getClassDefinitionRaw() const noexcept
        {
            return classDefinition.get();
        }
        void setField(const std::string& fieldName, const Value& value);

        // Get all field values (for debugging/inspection)
        const std::unordered_map<std::string, Value>& getAllFieldValues() const { return fieldValues; }

        // GC: Clear all field values to break reference cycles
        void clearAllFields() { fieldValues.clear(); }
        // Type checking
        bool isInstanceOf(const std::string& className) const;
        std::string getTypeName() const;
        std::string getFullTypeName() const;  // Returns full generic type name (e.g., "Box<String>")

        // NEW: Polymorphic method lookup with inheritance support
        std::shared_ptr<MethodDefinition> findMethodInHierarchy(const std::string& methodName, size_t argCount) const;
        
        // Generate content-based hash for Set/Map operations
        std::string getContentHash() const;
        
        // Content-based equality comparison for collections
        bool contentEquals(const ObjectInstance& other) const;

        // Phase 6 (IC): Indexed field access for inline caching
        void ensureFieldVector() const;
        const Value& getFieldByIndex(size_t index) const;
        void setFieldByIndex(size_t index, const Value& value);
        bool hasFieldVector() const { return fieldVectorInitialized; }

        // Fast primitive type tag (avoids string comparisons in hot paths)
        value::PrimitiveTypeTag getPrimitiveTag() const { return primitiveTag_; }

        // Generic type binding management
        void setGenericTypeBinding(const std::string& parameter, const std::string& concreteType);
        std::string resolveGenericType(const std::string& typeName) const;
        const std::unordered_map<std::string, std::string>& getGenericTypeBindings() const;

        // MYT-169: byte offset of the classDefinition shared_ptr from the start
        // of an ObjectInstance, consumed by JIT shape-guard emission to bypass
        // the jit_extract_classdef helper call. Defined out-of-line in the .cpp
        // where offsetof on this non-standard-layout type is guarded.
        static size_t classDefinitionMemberOffset() noexcept;

        // MYT-171: recycle hooks used by value::ObjectInstancePool. resetForRecycle
        // clears per-instance state (field values, method cache, generic bindings)
        // but keeps the unordered_map bucket arrays alive so the next acquire on
        // this slot avoids re-allocating them. reinitForRecycle re-binds the slot
        // to a fresh classDefinition + generic bindings without re-constructing
        // the maps.
        void resetForRecycle();
        void reinitForRecycle(std::shared_ptr<ClassDefinition> classDef,
                              const std::unordered_map<std::string, std::string>& typeBindings);
    };
}
