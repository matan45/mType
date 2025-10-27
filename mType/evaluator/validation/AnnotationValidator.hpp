#pragma once

#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/klass/MethodDefinition.hpp"
#include "../../errors/SourceLocation.hpp"
#include "../../environment/Environment.hpp"
#include <memory>
#include <string>

namespace evaluator::validation
{
    using namespace runtimeTypes::klass;
    using namespace errors;
    using namespace environment;

    /**
     * @brief Validator for annotation-based constraints
     *
     * Validates annotations such as @Override to ensure they are used correctly
     * according to inheritance and interface implementation rules.
     */
    class AnnotationValidator
    {
    public:
        /**
         * @brief Validate all annotations on a class and its members
         *
         * @param classDefinition The class definition to validate
         * @param environment The environment containing class registry
         * @throws InheritanceException if @Override is used incorrectly
         */
        static void validateClassAnnotations(
            std::shared_ptr<ClassDefinition> classDefinition,
            std::shared_ptr<Environment> environment);

        /**
         * @brief Validate @Override annotation on a method
         *
         * Checks that:
         * 1. The method actually overrides a parent method or implements an interface
         * 2. The method signature matches the parent/interface signature
         * 3. Access modifiers are not narrowed
         *
         * @param method The method to validate
         * @param containingClass The class that contains this method
         * @param environment The environment containing class and interface registries
         * @param location Source location for error reporting
         * @throws InheritanceException if @Override is used incorrectly
         */
        static void validateOverrideAnnotation(
            const MethodDefinition* method,
            std::shared_ptr<ClassDefinition> containingClass,
            std::shared_ptr<Environment> environment,
            const SourceLocation& location);

    private:
        /**
         * @brief Find a matching method in the parent class hierarchy
         *
         * @param method The method to search for
         * @param parentClass The parent class to search in
         * @return Pointer to matching parent method, or nullptr if not found
         */
        static std::shared_ptr<MethodDefinition> findMatchingMethodInParent(
            const MethodDefinition* method,
            std::shared_ptr<ClassDefinition> parentClass);

        /**
         * @brief Find a matching method signature in implemented interfaces
         *
         * @param method The method to search for
         * @param containingClass The class implementing the interfaces
         * @param environment The environment containing interface registry
         * @return true if a matching interface method is found
         */
        static bool findMatchingMethodInInterfaces(
            const MethodDefinition* method,
            std::shared_ptr<ClassDefinition> containingClass,
            std::shared_ptr<Environment> environment);

        /**
         * @brief Check if two method signatures match
         *
         * Compares method names, parameter counts, and parameter types.
         *
         * @param method1 First method
         * @param method2 Second method
         * @return true if signatures match
         */
        static bool methodSignaturesMatch(
            const MethodDefinition* method1,
            const MethodDefinition* method2);

        /**
         * @brief Generate descriptive error message for invalid @Override
         *
         * @param methodName The method with invalid @Override annotation
         * @param className The containing class name
         * @return Formatted error message
         */
        static std::string generateOverrideErrorMessage(
            const std::string& methodName,
            const std::string& className);
    };
}
