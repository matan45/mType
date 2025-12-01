#pragma once

#include "UnifiedType.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

namespace ast
{
    struct GenericTypeParameter;
}

namespace environment::registry
{
    class ClassRegistry;
}

namespace runtimeTypes::klass
{
    class InterfaceRegistry;
    class ClassDefinition;
    class InterfaceDefinition;
}

namespace types
{
    /**
     * Result of a constraint validation operation.
     */
    struct ValidationResult
    {
        bool valid;
        std::vector<std::string> errors;

        ValidationResult() : valid(true) {}
        explicit ValidationResult(bool isValid) : valid(isValid) {}

        static ValidationResult success() { return ValidationResult(true); }

        static ValidationResult failure(const std::string& error)
        {
            ValidationResult result(false);
            result.errors.push_back(error);
            return result;
        }

        static ValidationResult failure(std::vector<std::string> errs)
        {
            ValidationResult result(false);
            result.errors = std::move(errs);
            return result;
        }

        void addError(const std::string& error)
        {
            valid = false;
            errors.push_back(error);
        }

        bool isValid() const { return valid; }
        const std::vector<std::string>& getErrors() const { return errors; }

        std::string getErrorMessage() const;
    };

    /**
     * Validates that type arguments satisfy generic parameter constraints.
     *
     * Enforces bounded generics at compile time:
     * - T extends SomeClass: Validates that type argument extends/is SomeClass
     * - T implements SomeInterface: Validates that type argument implements interface
     *
     * Example:
     *   class SortedList<T extends Comparable> { ... }
     *   SortedList<String> is valid (String implements Comparable)
     *   SortedList<Object> is invalid (Object doesn't implement Comparable)
     */
    class TypeConstraintValidator
    {
    public:
        TypeConstraintValidator(
            std::shared_ptr<environment::registry::ClassRegistry> classReg,
            std::shared_ptr<runtimeTypes::klass::InterfaceRegistry> interfaceReg);

        ~TypeConstraintValidator() = default;

        // Non-copyable
        TypeConstraintValidator(const TypeConstraintValidator&) = delete;
        TypeConstraintValidator& operator=(const TypeConstraintValidator&) = delete;

        /**
         * Validates that concrete type arguments satisfy generic parameter constraints.
         *
         * @param parameters Generic type parameters with constraints
         * @param arguments Concrete type arguments to validate
         * @return ValidationResult with any constraint violations
         *
         * Example:
         *   parameters: [GenericTypeParameter("T", constraints=["Comparable"])]
         *   arguments: [classType("String")]
         *   result: valid if String implements Comparable
         */
        ValidationResult validateTypeArguments(
            const std::vector<ast::GenericTypeParameter>& parameters,
            const std::vector<UnifiedTypePtr>& arguments) const;

        /**
         * Validates a single type against a single constraint.
         *
         * @param concreteType The concrete type to check
         * @param constraint The constraint that must be satisfied
         * @return true if the type satisfies the constraint
         */
        bool satisfiesConstraint(
            const UnifiedTypePtr& concreteType,
            const TypeConstraint& constraint) const;

        /**
         * Checks if a type extends a given class.
         *
         * @param type The type to check
         * @param baseClassName The class that must be extended
         * @return true if type extends baseClassName
         */
        bool extendsClass(
            const UnifiedTypePtr& type,
            const std::string& baseClassName) const;

        /**
         * Checks if a type implements a given interface.
         *
         * @param type The type to check
         * @param interfaceName The interface that must be implemented
         * @return true if type implements interfaceName
         */
        bool implementsInterface(
            const UnifiedTypePtr& type,
            const std::string& interfaceName) const;

        /**
         * Checks if a type is assignable to another type.
         * Considers inheritance and interface implementation.
         *
         * @param source The source type (being assigned from)
         * @param target The target type (being assigned to)
         * @return true if source is assignable to target
         */
        bool isAssignableTo(
            const UnifiedTypePtr& source,
            const UnifiedTypePtr& target) const;

        /**
         * Gets all interfaces implemented by a type.
         */
        std::unordered_set<std::string> getAllImplementedInterfaces(
            const UnifiedTypePtr& type) const;

        /**
         * Gets the full inheritance chain for a class type.
         */
        std::vector<std::string> getInheritanceChain(
            const UnifiedTypePtr& type) const;

    private:
        std::shared_ptr<environment::registry::ClassRegistry> classRegistry;
        std::shared_ptr<runtimeTypes::klass::InterfaceRegistry> interfaceRegistry;

        /**
         * Validates a single constraint for a parameter/argument pair.
         */
        ValidationResult validateSingleConstraint(
            const std::string& paramName,
            const UnifiedTypePtr& argument,
            const std::string& constraintStr) const;

        /**
         * Parses a constraint string to extract type name.
         * e.g., "Comparable" or "Comparable<T>"
         */
        std::string extractConstraintTypeName(const std::string& constraint) const;

        /**
         * Gets interfaces implemented by a class definition.
         */
        void collectInterfacesFromClass(
            const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
            std::unordered_set<std::string>& interfaces) const;

        /**
         * Recursively collects interfaces including parent interfaces.
         */
        void collectParentInterfaces(
            const std::string& interfaceName,
            std::unordered_set<std::string>& interfaces,
            std::unordered_set<std::string>& visited) const;
    };
}
