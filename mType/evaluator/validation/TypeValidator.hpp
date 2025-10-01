#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../value/ValueType.hpp"
#include "../../errors/SourceLocation.hpp"
#include <string>
#include <memory>

namespace evaluator {
namespace validation {

    using namespace base;
    using namespace value;

    /**
     * @brief Unified type validation service for all evaluators
     *
     * This class provides consistent type checking and validation logic
     * across all evaluator components, following DDD Domain Service pattern.
     *
     * Responsibilities:
     * - Type compatibility checking
     * - Type conversion validation
     * - Object type compatibility
     * - Generic type compatibility
     * - Class existence validation
     *
     * Design Principles:
     * - Single Responsibility: Only type validation logic
     * - Stateless: All methods are static or use passed context
     * - DRY: Eliminates duplicate validation code across evaluators
     */
    class TypeValidator {
    public:
        /**
         * Validate type assignment (basic version without class name)
         * @throws TypeException if types are incompatible
         */
        static void validateAssignment(
            ValueType expectedType,
            const Value& actualValue,
            const std::string& variableName,
            const SourceLocation& location);

        /**
         * Validate type assignment with class name (for object types)
         * @throws TypeException if types are incompatible
         */
        static void validateAssignment(
            ValueType expectedType,
            const Value& actualValue,
            const std::string& variableName,
            const SourceLocation& location,
            const std::string& expectedClassName);

        /**
         * Validate function return type
         * @throws TypeException if return type doesn't match expected type
         */
        static void validateFunctionReturn(
            ValueType expectedType,
            const Value& returnValue,
            const std::string& functionName,
            const SourceLocation& location);

        /**
         * Validate that a class or interface exists in the environment
         * @throws UndefinedException if class/interface not found
         */
        static void validateClassExists(
            const std::string& className,
            const SourceLocation& location,
            std::shared_ptr<EvaluationContext> context);

        /**
         * Validate object type compatibility (class/interface matching)
         * @throws TypeException if object types are incompatible
         */
        static void validateObjectTypeCompatibility(
            const Value& value,
            const std::string& variableName,
            const SourceLocation& location,
            const std::string& expectedClassName,
            std::shared_ptr<EvaluationContext> context);

        /**
         * Check if one type can be implicitly converted to another
         * @return true if conversion is valid
         */
        static bool isValidTypeConversion(ValueType from, ValueType to);

        /**
         * Check if generic types are compatible (e.g., Set<int> vs Set<T>)
         * @return true if types are compatible
         */
        static bool isGenericTypeCompatible(
            const std::string& actualClassName,
            const std::string& expectedClassName);

    private:
        /**
         * Helper to resolve generic type parameters from context
         */
        static std::string resolveGenericClassName(
            const std::string& className,
            std::shared_ptr<EvaluationContext> context);

        /**
         * Helper to extract base class name from generic type
         */
        static std::string extractBaseClassName(const std::string& className);
    };

} // namespace validation
} // namespace evaluator
