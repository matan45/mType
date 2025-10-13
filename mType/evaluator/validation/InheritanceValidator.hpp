#pragma once

#include "../base/EvaluationContext.hpp"
#include "../../circularDependency/CircularDependencyDetector.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../ast/nodes/classes/ClassNode.hpp"
#include "../../errors/SourceLocation.hpp"
#include <string>
#include <memory>
#include <vector>

namespace evaluator {
namespace validation {

    using namespace base;
    using namespace runtimeTypes::klass;
    using namespace circularDependency;

    /**
     * @brief Validator for inheritance-related semantic rules
     *
     * This class validates inheritance constraints:
     * - Parent class existence
     * - No circular inheritance
     * - Method override compatibility
     * - Super call validity
     *
     * Design Principles:
     * - Stateless validation methods
     * - Clear error messages with context
     * - Integration with CircularDependencyDetector
     */
    class InheritanceValidator {
    public:
        /**
         * Validate that parent class exists in the environment
         * @throws InheritanceException if parent class not found
         */
        static void validateParentClassExists(
            const std::string& parentClassName,
            const SourceLocation& location,
            std::shared_ptr<EvaluationContext> context);

        /**
         * Validate no circular inheritance using CircularDependencyDetector
         * @throws InheritanceException if circular inheritance detected
         */
        static void validateCircularInheritance(
            const std::string& childClassName,
            const std::string& parentClassName,
            const SourceLocation& location,
            CircularDependencyDetector& detector);

        /**
         * Validate method override signatures match exactly
         * @throws InheritanceException if override signature doesn't match parent
         */
        static void validateMethodOverride(
            const std::string& childClassName,
            const std::string& parentClassName,
            std::shared_ptr<MethodDefinition> childMethod,
            std::shared_ptr<MethodDefinition> parentMethod,
            const SourceLocation& location);

        /**
         * Validate all method overrides in a class
         * @throws InheritanceException if any override is invalid
         */
        static void validateMethodOverrides(
            std::shared_ptr<ClassDefinition> childClass,
            std::shared_ptr<ClassDefinition> parentClass,
            const SourceLocation& location);

        // Note: super() constructor call validation is NOT needed here.
        // The parser enforces that super() must use initializer list syntax:
        //   constructor(params) : super(args) { body }
        // This guarantees super() executes before the constructor body,
        // preventing use-before-initialization bugs at parse time.
        // See ConstructorParser.cpp:46-88 for implementation.

        /**
         * Validate super.method() call
         * - Parent must have the method
         * - Method must not be private
         * @throws InheritanceException if super method call is invalid
         */
        static void validateSuperMethodCall(
            const std::string& methodName,
            size_t argCount,
            const SourceLocation& location,
            std::shared_ptr<EvaluationContext> context);

        /**
         * Validate inheritance chain depth
         * @throws InheritanceException if chain exceeds maximum depth
         */
        static void validateInheritanceDepth(
            const std::string& className,
            const SourceLocation& location,
            std::shared_ptr<EvaluationContext> context);

        // Note: The following validations are now handled by the parser at compile-time:
        // - validateClassCannotExtendInterface (ClassDeclarationParser.cpp:92-98)
        // - validateInterfaceCannotExtendClass (InterfaceParser.cpp:130-136)
        // - validateParentClassNotFinal (ClassDeclarationParser.cpp:101-106)
        // - validateParentInterfaceNotFinal (InterfaceParser.cpp:139-144)

    private:
        static constexpr int MAX_INHERITANCE_DEPTH = 20;

        /**
         * Helper to check if method signatures match
         */
        static bool methodSignaturesMatch(
            std::shared_ptr<MethodDefinition> method1,
            std::shared_ptr<MethodDefinition> method2);

        /**
         * Helper to get method signature string for error messages
         */
        static std::string getMethodSignature(std::shared_ptr<MethodDefinition> method);
    };

} // namespace validation
} // namespace evaluator
