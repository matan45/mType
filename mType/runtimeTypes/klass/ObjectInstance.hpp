#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "../../value/ValueType.hpp"
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
        static bool compareFieldValues(const Value& thisValue, const Value& otherValue);

        // GC: Flag to track if this instance is registered with GC
        bool gcRegistered = false;

    public :
        ObjectInstance(std::shared_ptr<ClassDefinition> classDef)
            : classDefinition(classDef)
        {
            // PERFORMANCE: Pre-size field map to avoid rehashing during field initialization
            if (classDef) {
                fieldValues.reserve(classDef->getTotalFieldCount());
            }
        }

        // Constructor with generic type bindings
        ObjectInstance(std::shared_ptr<ClassDefinition> classDef,
                      const std::unordered_map<std::string, std::string>& typeBindings)
            : classDefinition(classDef), genericTypeBindings(typeBindings)
        {
            // PERFORMANCE: Pre-size field map to avoid rehashing during field initialization
            if (classDef) {
                fieldValues.reserve(classDef->getTotalFieldCount());
            }
        }

        // GC: Register this instance with garbage collector
        // Call this after creating a shared_ptr to the instance
        void registerWithGC();

        std::shared_ptr<FieldDefinition> getField(const std::string& fieldName) const;
        Value getFieldValue(const std::string& fieldName) const;
        std::shared_ptr<ClassDefinition> getClassDefinition() const;
        void setField(const std::string& fieldName, const Value& value);

        // Get all field values (for debugging/inspection)
        const std::unordered_map<std::string, Value>& getAllFieldValues() const { return fieldValues; }
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

        // Generic type binding management
        void setGenericTypeBinding(const std::string& parameter, const std::string& concreteType);
        std::string resolveGenericType(const std::string& typeName) const;
        const std::unordered_map<std::string, std::string>& getGenericTypeBindings() const;
    };
}
