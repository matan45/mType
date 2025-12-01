#pragma once

#include "UnifiedType.hpp"
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <vector>
#include <memory>

namespace runtimeTypes::klass
{
    class ObjectInstance;
}

namespace types
{
    /**
     * Registry for reified (runtime-preserved) generic types.
     *
     * Unlike type erasure (Java), mType preserves full generic type information
     * at runtime. This enables:
     * - Runtime type checks: obj instanceof Container<String>
     * - Reflection on type arguments
     * - Distinct identity for Container<String> vs Container<Int>
     *
     * The registry ensures that identical parameterized types share the same
     * UnifiedTypePtr instance (structural sharing/interning).
     */
    class ReifiedTypeRegistry
    {
    public:
        ReifiedTypeRegistry() = default;
        ~ReifiedTypeRegistry() = default;

        // Non-copyable, non-movable
        ReifiedTypeRegistry(const ReifiedTypeRegistry&) = delete;
        ReifiedTypeRegistry& operator=(const ReifiedTypeRegistry&) = delete;

        /**
         * Interns a type, returning the canonical instance.
         *
         * If an equivalent type already exists in the registry, returns that.
         * Otherwise, stores and returns the provided type.
         *
         * This ensures Container<String> always references the same object.
         *
         * @param type The type to intern
         * @return Canonical type instance
         */
        UnifiedTypePtr intern(const UnifiedTypePtr& type);

        /**
         * Checks if a type is already interned.
         */
        bool isInterned(const UnifiedTypePtr& type) const;

        /**
         * Gets an interned type by its canonical string representation.
         *
         * @param canonicalString The canonical type string (e.g., "Container<String>")
         * @return The interned type, or nullptr if not found
         */
        UnifiedTypePtr findByCanonicalString(const std::string& canonicalString) const;

        /**
         * Checks if an object is an instance of a specific parameterized type.
         *
         * Unlike simple class checks, this validates type arguments:
         *   obj instanceof Container<String> - checks both class and type args
         *
         * @param obj The object to check
         * @param type The expected type (may be parameterized)
         * @return true if obj is an instance of type
         */
        bool isInstance(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj,
            const UnifiedTypePtr& type) const;

        /**
         * Gets the reified type of an object.
         *
         * For Container<String> instance, returns Container<String> (not just Container).
         *
         * @param obj The object to query
         * @return The object's reified type, or nullptr if unknown
         */
        UnifiedTypePtr getReifiedType(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) const;

        /**
         * Associates an object with its reified type.
         *
         * Called when creating instances of generic classes.
         *
         * @param obj The object instance
         * @param type The reified type (with concrete type arguments)
         */
        void registerObjectType(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj,
            const UnifiedTypePtr& type);

        /**
         * Removes the type registration for an object.
         *
         * Called when objects are destroyed.
         */
        void unregisterObject(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj);

        /**
         * Gets all type arguments for an object.
         *
         * For Container<String> instance, returns [String].
         *
         * @param obj The object to query
         * @return Vector of type arguments, empty if not parameterized
         */
        std::vector<UnifiedTypePtr> getTypeArguments(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj) const;

        /**
         * Checks if two objects have the same reified type.
         */
        bool haveSameReifiedType(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj1,
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj2) const;

        /**
         * Gets all currently interned types.
         */
        std::vector<UnifiedTypePtr> getAllInternedTypes() const;

        /**
         * Gets the number of interned types.
         */
        size_t getInternedTypeCount() const;

        /**
         * Gets the number of registered object instances.
         */
        size_t getRegisteredObjectCount() const;

        /**
         * Clears all interned types and object registrations.
         * Use with caution - primarily for testing.
         */
        void clear();

        /**
         * Performs garbage collection on orphaned object registrations.
         *
         * Removes registrations for objects that have been destroyed
         * (weak_ptr expired).
         *
         * @return Number of registrations cleaned up
         */
        size_t cleanupOrphanedRegistrations();

    private:
        // Type interning cache: canonical string -> type
        mutable std::mutex typeCacheMutex;
        std::unordered_map<std::string, UnifiedTypePtr> typeCache;

        // Object type registrations: object pointer -> reified type
        // Uses weak_ptr to avoid preventing object destruction
        mutable std::mutex objectRegistryMutex;
        std::unordered_map<const void*, std::pair<std::weak_ptr<runtimeTypes::klass::ObjectInstance>, UnifiedTypePtr>> objectTypes;

        /**
         * Checks type compatibility for instanceof checks.
         * Handles parameterized types, inheritance, and interface implementation.
         */
        bool isTypeCompatible(
            const UnifiedTypePtr& objectType,
            const UnifiedTypePtr& expectedType) const;
    };
}
