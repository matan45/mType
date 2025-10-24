#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../errors/SourceLocation.hpp"
#include <string>
#include <memory>
#include <vector>

namespace evaluator {
namespace validation {

    using namespace base;
    using namespace runtimeTypes::klass;

    /**
     * @brief Validator for abstract class-related semantic rules
     *
     * This class validates abstract class constraints:
     * - Abstract classes cannot be instantiated
     * - Concrete classes must implement all abstract methods
     * - Abstract methods have proper signatures
     *
     * Design Principles:
     * - Stateless validation methods
     * - Clear error messages with context
     * - Integration with InheritanceValidator
     */
    class AbstractClassValidator {
    public:
        /**
         * Validate that the class being instantiated is not abstract
         * @throws AbstractClassException if trying to instantiate abstract class
         */
        static void validateAbstractClassNotInstantiated(
            std::shared_ptr<ClassDefinition> classDef,
            const SourceLocation& location);

        /**
         * Validate that a concrete class implements all abstract methods
         * from its inheritance hierarchy
         * @throws AbstractClassException if abstract methods are not implemented
         */
        static void validateAllAbstractMethodsImplemented(
            std::shared_ptr<ClassDefinition> classDef,
            const SourceLocation& location);

        /**
         * Validate that an abstract method has no body (parser should catch this)
         * @throws AbstractClassException if abstract method has a body
         */
        static void validateAbstractMethodHasNoBody(
            std::shared_ptr<MethodDefinition> method,
            const std::string& className,
            const SourceLocation& location);

        /**
         * Validate that only abstract classes can have abstract methods
         * @throws AbstractClassException if non-abstract class has abstract methods
         */
        static void validateAbstractMethodsOnlyInAbstractClass(
            std::shared_ptr<ClassDefinition> classDef,
            const SourceLocation& location);

        /**
         * Validate that a concrete class is complete (all abstract methods implemented)
         * Called during class registration
         * @throws AbstractClassException if concrete class has unimplemented methods
         */
        static void validateConcreteClassIsComplete(
            std::shared_ptr<ClassDefinition> classDef,
            const SourceLocation& location);

        /**
         * Get list of unimplemented abstract methods for error reporting
         */
        static std::vector<std::string> getUnimplementedMethods(
            std::shared_ptr<ClassDefinition> classDef);

    private:
        /**
         * Helper to format unimplemented methods list for error messages
         */
        static std::string formatUnimplementedMethodsList(
            const std::vector<std::string>& methods);
    };

} // namespace validation
} // namespace evaluator
