#pragma once

#include "../../value/ValueType.hpp"
#include <string>
#include <memory>

namespace evaluator {
namespace utils {

    using namespace value;

    /**
     * @brief Object instance helper utilities
     *
     * This class provides common operations for object instances,
     * centralizing object-related helper logic across evaluators.
     *
     * Responsibilities:
     * - Extract class names from object values
     * - Validate object type compatibility
     * - Check instance relationships
     *
     * Design Principles:
     * - Single Responsibility: Only object instance helpers
     * - Stateless: All methods are static
     * - DRY: Eliminates duplicate object handling code
     */
    class ObjectHelper {
    public:
        /**
         * Get the class name from an object value
         * @param objectValue The value to extract class name from
         * @return The class name, or "unknown" if not an object
         */
        static std::string getClassName(const Value& objectValue);

        /**
         * Check if an object is an instance of a specific class
         * @param objectValue The object value to check
         * @param className The class name to match
         * @return true if the object is an instance of className
         */
        static bool isInstanceOf(const Value& objectValue, const std::string& className);

        /**
         * Validate that two object values have compatible types
         * @param expected The expected value
         * @param actual The actual value
         * @return true if the types are compatible
         */
        static bool areObjectTypesCompatible(const Value& expected, const Value& actual);

        /**
         * Extract ObjectInstance pointer from a Value
         * @param value The value containing the object
         * @return Shared pointer to ObjectInstance, or nullptr if not an object
         */
        static std::shared_ptr<runtimeTypes::klass::ObjectInstance> extractInstance(const Value& value);
    };

} // namespace utils
} // namespace evaluator
