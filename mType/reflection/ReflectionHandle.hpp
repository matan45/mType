#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <atomic>
#include <vector>
#include <cstdint>
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../runtimeTypes/klass/MethodDefinition.hpp"
#include "../runtimeTypes/klass/ConstructorDefinition.hpp"
#include "../ast/nodes/annotations/AnnotationNode.hpp"
#include "../types/UnifiedType.hpp"

namespace reflection
{
    /**
     * @brief Type tag for efficient handle type lookup
     */
    enum class HandleType : uint8_t
    {
        CLASS,
        FIELD,
        METHOD,
        CONSTRUCTOR,
        ANNOTATION
    };

    /**
     * @brief Information associated with a field handle
     */
    struct FieldHandleInfo
    {
        std::shared_ptr<runtimeTypes::klass::FieldDefinition> field;
        int64_t classHandle;
        std::string fieldName;
    };

    /**
     * @brief Information associated with a method handle
     */
    struct MethodHandleInfo
    {
        std::shared_ptr<runtimeTypes::klass::MethodDefinition> method;
        int64_t classHandle;
        std::string methodName;
    };

    /**
     * @brief Information associated with a constructor handle
     */
    struct ConstructorHandleInfo
    {
        std::shared_ptr<runtimeTypes::klass::ConstructorDefinition> constructor;
        int64_t classHandle;
        int constructorIndex;
    };

    /**
     * @brief Information associated with an annotation handle
     */
    struct AnnotationHandleInfo
    {
        std::shared_ptr<ast::nodes::annotations::AnnotationNode> annotation;
        std::string annotationName;
    };

    /**
     * @brief Manages opaque integer handles for reflection objects
     *
     * This registry maintains mappings between integer handles and the actual
     * runtime type definitions (ClassDefinition, FieldDefinition, etc.).
     * Handles are used by the mType reflection API to reference C++ objects
     * without exposing internal pointers.
     *
     * Thread-safe singleton implementation.
     */
    class ReflectionHandleRegistry
    {
    private:
        std::atomic<int64_t> nextHandle{1};

        // Handle -> ClassDefinition mapping
        std::unordered_map<int64_t, std::shared_ptr<runtimeTypes::klass::ClassDefinition>> classHandles;

        // Reverse mapping: class name -> handle (for fast lookup)
        std::unordered_map<std::string, int64_t> classNameToHandle;

        // Handle -> FieldHandleInfo mapping
        std::unordered_map<int64_t, FieldHandleInfo> fieldHandles;

        // Handle -> MethodHandleInfo mapping
        std::unordered_map<int64_t, MethodHandleInfo> methodHandles;

        // Handle -> ConstructorHandleInfo mapping
        std::unordered_map<int64_t, ConstructorHandleInfo> constructorHandles;

        // Handle -> AnnotationHandleInfo mapping
        std::unordered_map<int64_t, AnnotationHandleInfo> annotationHandles;

        // ========== Reverse Mapping Caches for Deduplication ==========
        // These prevent unbounded handle growth by reusing existing handles

        // "classHandle:fieldName" -> handle
        std::unordered_map<std::string, int64_t> fieldKeyToHandle;

        // "classHandle:methodName(param1,param2,...)" -> handle
        std::unordered_map<std::string, int64_t> methodKeyToHandle;

        // "classHandle:ctor:index" -> handle
        std::unordered_map<std::string, int64_t> constructorKeyToHandle;

        // annotation pointer address -> handle
        std::unordered_map<std::uintptr_t, int64_t> annotationPtrToHandle;

        // ========== Handle Type Tracking for O(1) Release ==========
        // Maps handle -> type for efficient releaseHandle lookup
        std::unordered_map<int64_t, HandleType> handleTypeMap;

        // ========== Closed-Form (Parameterized) Class Handles ==========
        // For handles that represent a closed parameterized type like Box<Int>.
        // Populated ONLY for closed handles; open-form class handles have no entry here.
        // The handle still lives in classHandles (pointing at the base ClassDefinition)
        // so every existing reflection native continues to work unchanged.
        std::unordered_map<int64_t, ::types::UnifiedTypePtr> closedHandleReified;

        // Reverse dedup map keyed by UnifiedType::toCanonicalString() (e.g. "Box<Int>").
        // Disjoint from classNameToHandle (keyed on raw names like "Box"), which
        // automatically gives `forName("Box") != forName("Box<Int>")` for free.
        std::unordered_map<std::string, int64_t> canonicalStringToHandle;

        // ========== Private Helper Methods ==========
        static std::string makeFieldKey(int64_t classHandle, const std::string& fieldName);
        static std::string makeMethodKey(int64_t classHandle, const std::string& methodName,
                                         const std::vector<std::string>& paramTypes);
        static std::string makeConstructorKey(int64_t classHandle, int constructorIndex);

        // Private constructor for singleton
        ReflectionHandleRegistry() = default;

        // Destructor - clears all maps to avoid issues with static destruction order
        ~ReflectionHandleRegistry()
        {
            // Clear all maps to release shared_ptr references before other static objects are destroyed
            classHandles.clear();
            classNameToHandle.clear();
            fieldHandles.clear();
            methodHandles.clear();
            constructorHandles.clear();
            annotationHandles.clear();
            // Clear reverse mapping caches
            fieldKeyToHandle.clear();
            methodKeyToHandle.clear();
            constructorKeyToHandle.clear();
            annotationPtrToHandle.clear();
            // Clear handle type tracking
            handleTypeMap.clear();
            // Clear closed-form parameterized class handle maps
            closedHandleReified.clear();
            canonicalStringToHandle.clear();
        }

    public:
        // Delete copy/move constructors
        ReflectionHandleRegistry(const ReflectionHandleRegistry&) = delete;
        ReflectionHandleRegistry& operator=(const ReflectionHandleRegistry&) = delete;
        ReflectionHandleRegistry(ReflectionHandleRegistry&&) = delete;
        ReflectionHandleRegistry& operator=(ReflectionHandleRegistry&&) = delete;

        /**
         * @brief Get the singleton instance
         */
        static ReflectionHandleRegistry& instance();

        // ========== Class Handle Management ==========

        /**
         * @brief Register a class definition and get a handle
         * @param classDef The class definition to register
         * @return Integer handle for the class
         */
        int64_t registerClass(const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef);

        /**
         * @brief Get class definition by handle
         * @param handle The class handle
         * @return ClassDefinition or nullptr if not found
         */
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> getClass(int64_t handle) const;

        /**
         * @brief Find existing handle for a class by name
         * @param className The fully qualified class name
         * @return Handle if found, -1 otherwise
         */
        int64_t findClassHandle(const std::string& className) const;

        /**
         * @brief Get or create a handle for a class
         * @param classDef The class definition
         * @return Existing or new handle
         */
        int64_t getOrCreateClassHandle(const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef);

        /**
         * @brief Get or create a handle for a closed parameterized class type
         *
         * Returns a handle distinct from the open-form handle of the same base
         * class. For example, `forName("Box")` and `forName("Box<Int>")` produce
         * different handles via different maps.
         *
         * The returned handle still resolves to the base ClassDefinition through
         * getClass(), so all existing reflection natives (getFields, getMethods,
         * etc.) continue to work unchanged on closed handles.
         *
         * @param baseDef The base ClassDefinition (e.g. the `Box` definition)
         * @param reified The reified UnifiedType describing the closed form
         *                (e.g. `Box<Int>`). Will be interned via ReifiedTypeRegistry.
         * @return Existing or new handle (interned by canonical string)
         */
        int64_t getOrCreateClosedClassHandle(
            const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& baseDef,
            const ::types::UnifiedTypePtr& reified);

        /**
         * @brief Get the reified UnifiedType for a closed class handle
         * @param handle The class handle
         * @return The reified type if the handle is closed, nullptr for open
         *         handles or unknown handles
         */
        ::types::UnifiedTypePtr getReifiedType(int64_t handle) const;

        /**
         * @brief Check if a handle represents a closed (parameterized) class form
         * @param handle The class handle
         * @return true if a reified type is associated with this handle
         */
        bool isClosedHandle(int64_t handle) const;

        // ========== Field Handle Management ==========

        /**
         * @brief Register a field definition and get a handle
         * @param field The field definition
         * @param classHandle Handle of the owning class
         * @param fieldName Name of the field
         * @return Integer handle for the field
         */
        int64_t registerField(const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field,
                         int64_t classHandle, const std::string& fieldName);

        /**
         * @brief Get field info by handle
         * @param handle The field handle
         * @return FieldHandleInfo (field may be nullptr if not found)
         */
        FieldHandleInfo getField(int64_t handle) const;

        // ========== Method Handle Management ==========

        /**
         * @brief Register a method definition and get a handle
         * @param method The method definition
         * @param classHandle Handle of the owning class
         * @param methodName Name of the method
         * @return Integer handle for the method
         */
        int64_t registerMethod(const std::shared_ptr<runtimeTypes::klass::MethodDefinition>& method,
                          int64_t classHandle, const std::string& methodName);

        /**
         * @brief Get method info by handle
         * @param handle The method handle
         * @return MethodHandleInfo (method may be nullptr if not found)
         */
        MethodHandleInfo getMethod(int64_t handle) const;

        // ========== Constructor Handle Management ==========

        /**
         * @brief Register a constructor definition and get a handle
         * @param constructor The constructor definition
         * @param classHandle Handle of the owning class
         * @param constructorIndex Index in the class's constructor list
         * @return Integer handle for the constructor
         */
        int64_t registerConstructor(const std::shared_ptr<runtimeTypes::klass::ConstructorDefinition>& constructor,
                               int64_t classHandle, int constructorIndex);

        /**
         * @brief Get constructor info by handle
         * @param handle The constructor handle
         * @return ConstructorHandleInfo (constructor may be nullptr if not found)
         */
        ConstructorHandleInfo getConstructor(int64_t handle) const;

        // ========== Annotation Handle Management ==========

        /**
         * @brief Register an annotation and get a handle
         * @param annotation The annotation node
         * @param annotationName Name of the annotation
         * @return Integer handle for the annotation
         */
        int64_t registerAnnotation(const std::shared_ptr<ast::nodes::annotations::AnnotationNode>& annotation,
                              const std::string& annotationName);

        /**
         * @brief Get annotation info by handle
         * @param handle The annotation handle
         * @return AnnotationHandleInfo (annotation may be nullptr if not found)
         */
        AnnotationHandleInfo getAnnotation(int64_t handle) const;

        // ========== Utility Methods ==========

        /**
         * @brief Release a handle (for cleanup)
         * @param handle The handle to release
         */
        void releaseHandle(int64_t handle);

        /**
         * @brief Clear all handles (use with caution)
         */
        void clear();

        /**
         * @brief Get total number of registered handles
         */
        size_t getTotalHandleCount() const;
    };

} // namespace reflection
