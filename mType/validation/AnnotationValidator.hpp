#pragma once

#include "../environment/registry/ClassDefinition.hpp"
#include "../environment/registry/MethodDefinition.hpp"
#include "../errors/SourceLocation.hpp"
#include "../environment/Environment.hpp"
#include "../ast/nodes/annotations/AnnotationNode.hpp"
#include <memory>
#include <string>
#include <vector>

namespace validation
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

        /**
         * @brief Validate @Script annotation on a class
         *
         * Checks that:
         * 1. The class is not abstract
         * 2. The class has a default constructor (0 parameters)
         * 3. The class has an onUpdate(float deltaTime): void method
         * 4. The class has an onStart(): void method
         * 5. The class has an onDestroy(): void method
         *
         * @param classDefinition The class definition to validate
         * @param location Source location for error reporting
         * @throws TypeException if @Script requirements are not met
         */
        static void validateScriptAnnotation(
            std::shared_ptr<ClassDefinition> classDefinition,
            const SourceLocation& location);

        /**
         * @brief Validate @EntryPoint annotation on a class
         *
         * Checks that:
         * 1. The class is not abstract
         * 2. The class has a static main(args): void method
         * 3. The main method accepts exactly one parameter (String[] args)
         *
         * @param classDefinition The class definition to validate
         * @param location Source location for error reporting
         * @throws TypeException if @EntryPoint requirements are not met
         */
        static void validateEntryPointAnnotation(
            std::shared_ptr<ClassDefinition> classDefinition,
            const SourceLocation& location);

        /**
         * @brief Validate @Throw annotation on a method or function
         *
         * Checks that:
         * 1. All exception class names in the annotation exist in the class registry
         * 2. All exception classes inherit from the Exception base class
         * 3. No duplicate exception class names are declared
         *
         * @param throwAnnotation The @Throw annotation to validate
         * @param environment The environment containing class registry
         * @param location Source location for error reporting
         * @throws TypeException if exception class doesn't exist or is invalid
         */
        static void validateThrowAnnotation(
            std::shared_ptr<ast::nodes::annotations::AnnotationNode> throwAnnotation,
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

        /**
         * @brief Parse comma-separated exception class names from annotation parameter
         *
         * @param exceptionsParam The comma-separated string of exception class names
         * @return Vector of exception class names
         */
        static std::vector<std::string> parseExceptionList(const std::string& exceptionsParam);

        /**
         * @brief Check if a class inherits from the Exception base class
         *
         * @param className The class name to check
         * @param environment The environment containing class registry
         * @return true if the class is an exception class
         */
        static bool isExceptionClass(
            const std::string& className,
            std::shared_ptr<Environment> environment);

        /**
         * @brief Check for duplicate exception names in the list
         *
         * @param exceptionNames The list of exception class names
         * @param location Source location for error reporting
         * @throws TypeException if duplicates are found (as warning)
         */
        static void checkForDuplicates(
            const std::vector<std::string>& exceptionNames,
            const SourceLocation& location);
    };
}
