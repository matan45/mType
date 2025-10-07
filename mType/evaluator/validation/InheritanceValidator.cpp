#include "InheritanceValidator.hpp"
#include "../../errors/InheritanceException.hpp"
#include "../../errors/UndefinedException.hpp"
#include "../../value/ParameterType.hpp"
#include <sstream>

namespace evaluator {
namespace validation {

    using namespace errors;

    void InheritanceValidator::validateParentClassExists(
        const std::string& parentClassName,
        const SourceLocation& location,
        std::shared_ptr<EvaluationContext> context)
    {
        if (!context) {
            throw InheritanceException("No evaluation context available for validation", location);
        }

        auto env = context->getEnvironment();

        // Extract base class name from generic type (e.g., "Container<T>" -> "Container")
        std::string baseClassName = parentClassName;
        size_t genericStart = parentClassName.find('<');
        if (genericStart != std::string::npos) {
            baseClassName = parentClassName.substr(0, genericStart);
        }

        auto parentClass = env->findClass(baseClassName);

        if (!parentClass) {
            throw InheritanceException(
                "Parent class '" + parentClassName + "' does not exist",
                "", parentClassName, location);
        }
    }

    void InheritanceValidator::validateCircularInheritance(
        const std::string& childClassName,
        const std::string& parentClassName,
        const SourceLocation& location,
        CircularDependencyDetector& detector)
    {
        // Try to enter the dependency - this will detect cycles
        if (!detector.enterDependency(
                DependencyType::CLASS_INHERITANCE,
                childClassName,
                location.toString()))
        {
            // Circular dependency detected
            auto chain = detector.getDependencyChain(DependencyType::CLASS_INHERITANCE);

            std::stringstream chainMsg;
            chainMsg << "Circular inheritance detected: ";
            for (size_t i = 0; i < chain.size(); ++i) {
                if (i > 0) chainMsg << " -> ";
                chainMsg << chain[i];
            }
            chainMsg << " -> " << childClassName;

            throw InheritanceException(
                chainMsg.str(),
                childClassName,
                parentClassName,
                location);
        }

        // Exit the dependency when done
        detector.exitDependency(DependencyType::CLASS_INHERITANCE, childClassName);
    }

    void InheritanceValidator::validateMethodOverride(
        const std::string& childClassName,
        const std::string& parentClassName,
        std::shared_ptr<MethodDefinition> childMethod,
        std::shared_ptr<MethodDefinition> parentMethod,
        const SourceLocation& location)
    {
        if (!childMethod || !parentMethod) {
            return; // No override to validate
        }

        // Check if signatures match
        if (!methodSignaturesMatch(childMethod, parentMethod)) {
            std::string childSig = getMethodSignature(childMethod);
            std::string parentSig = getMethodSignature(parentMethod);

            throw InheritanceException(
                "Method override signature mismatch in class '" + childClassName +
                "': child method " + childSig +
                " does not match parent method " + parentSig,
                childClassName,
                parentClassName,
                childMethod->getName(),
                location);
        }

        // Check return type compatibility
        if (childMethod->getReturnType() != parentMethod->getReturnType()) {
            throw InheritanceException(
                "Method override return type mismatch in class '" + childClassName +
                "': method '" + childMethod->getName() + "' return type does not match parent method",
                childClassName,
                parentClassName,
                childMethod->getName(),
                location);
        }
    }

    void InheritanceValidator::validateMethodOverrides(
        std::shared_ptr<ClassDefinition> childClass,
        std::shared_ptr<ClassDefinition> parentClass,
        const SourceLocation& location)
    {
        if (!childClass || !parentClass) {
            return;
        }

        // Check each method in child class
        const auto& childMethods = childClass->getInstanceMethods();
        const auto& parentMethods = parentClass->getInstanceMethods();

        for (const auto& [methodName, childMethod] : childMethods) {
            auto parentIt = parentMethods.find(methodName);
            if (parentIt != parentMethods.end()) {
                // Method exists in parent - validate override
                validateMethodOverride(
                    childClass->getName(),
                    parentClass->getName(),
                    childMethod,
                    parentIt->second,
                    location);
            }
        }
    }

    void InheritanceValidator::validateSuperConstructorCall(
        const std::string& childClassName,
        const std::string& parentClassName,
        size_t argCount,
        bool isFirstStatement,
        const SourceLocation& location,
        std::shared_ptr<EvaluationContext> context)
    {
        // Validate super() is first statement
        if (!isFirstStatement) {
            throw InheritanceException(
                "super() constructor call must be the first statement in the constructor",
                childClassName,
                parentClassName,
                location);
        }

        // Validate parent class exists and has matching constructor
        auto env = context->getEnvironment();
        auto parentClass = env->findClass(parentClassName);

        if (!parentClass) {
            throw InheritanceException(
                "Parent class '" + parentClassName + "' not found for super() call",
                childClassName,
                parentClassName,
                location);
        }

        // Find matching constructor in parent
        auto parentConstructor = parentClass->findConstructor(argCount);
        if (!parentConstructor) {
            throw InheritanceException(
                "No matching constructor in parent class '" + parentClassName +
                "' with " + std::to_string(argCount) + " parameter(s)",
                childClassName,
                parentClassName,
                location);
        }
    }

    void InheritanceValidator::validateSuperMethodCall(
        const std::string& methodName,
        size_t argCount,
        const SourceLocation& location,
        std::shared_ptr<EvaluationContext> context)
    {
        // Get current instance to find parent class
        auto currentInstance = context->getCurrentInstance();
        if (!currentInstance) {
            throw InheritanceException(
                "super." + methodName + "() can only be called within an instance method",
                location);
        }

        auto currentClass = currentInstance->getClassDefinition();
        if (!currentClass->hasParentClass()) {
            throw InheritanceException(
                "Class '" + currentClass->getName() +
                "' has no parent class, cannot call super." + methodName + "()",
                location);
        }

        auto parentClass = currentClass->getParentClass();
        if (!parentClass) {
            throw InheritanceException(
                "Parent class not found for super." + methodName + "() call",
                location);
        }

        // Find method in parent
        auto parentMethod = parentClass->findMethod(methodName, argCount);
        if (!parentMethod) {
            throw InheritanceException(
                "Method '" + methodName + "' with " + std::to_string(argCount) +
                " parameter(s) not found in parent class '" + parentClass->getName() + "'",
                currentClass->getName(),
                parentClass->getName(),
                methodName,
                location);
        }

        // Note: MethodDefinition doesn't have isPrivate() - skip private check for now
        // In mType, all methods are effectively public if they exist in the parent class
    }

    void InheritanceValidator::validateInheritanceDepth(
        const std::string& className,
        const SourceLocation& location,
        std::shared_ptr<EvaluationContext> context)
    {
        auto env = context->getEnvironment();
        auto classRegistry = env->getClassRegistry();

        auto chain = classRegistry->getInheritanceChain(className);

        if (chain.size() >= MAX_INHERITANCE_DEPTH) {
            std::stringstream chainMsg;
            chainMsg << "Inheritance chain exceeds maximum depth of "
                     << MAX_INHERITANCE_DEPTH << ": " << className;

            for (const auto& parentClass : chain) {
                chainMsg << " -> " << parentClass->getName();
            }

            throw InheritanceException(chainMsg.str(), location);
        }
    }

    bool InheritanceValidator::methodSignaturesMatch(
        std::shared_ptr<MethodDefinition> method1,
        std::shared_ptr<MethodDefinition> method2)
    {
        if (!method1 || !method2) {
            return false;
        }

        // Check parameter count
        const auto& params1 = method1->getParameters();
        const auto& params2 = method2->getParameters();

        if (params1.size() != params2.size()) {
            return false;
        }

        // Check each parameter type
        for (size_t i = 0; i < params1.size(); ++i) {
            const auto& param1 = params1[i].second;  // Get ParameterType
            const auto& param2 = params2[i].second;

            // Compare using ParameterType equality operator
            if (!(param1 == param2)) {
                return false;
            }
        }

        return true;
    }

    std::string InheritanceValidator::getMethodSignature(std::shared_ptr<MethodDefinition> method)
    {
        if (!method) {
            return "null";
        }

        std::stringstream sig;
        sig << method->getName() << "(";

        const auto& params = method->getParameters();
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) sig << ", ";

            // Use ParameterType toString() method
            const auto& paramType = params[i].second;
            sig << paramType.toString();
        }

        sig << ")";
        return sig.str();
    }

    void InheritanceValidator::validateClassCannotExtendInterface(
        const std::string& className,
        const std::string& parentName,
        const SourceLocation& location,
        std::shared_ptr<EvaluationContext> context)
    {
        if (!context) {
            throw InheritanceException("No evaluation context available for validation", location);
        }

        auto env = context->getEnvironment();

        // Extract base parent name from generic type (e.g., "Container<T>" -> "Container")
        std::string baseParentName = parentName;
        size_t genericStart = parentName.find('<');
        if (genericStart != std::string::npos) {
            baseParentName = parentName.substr(0, genericStart);
        }

        // Check if the parent is actually an interface
        auto interfaceRegistry = env->getInterfaceRegistry();
        if (interfaceRegistry && interfaceRegistry->hasInterface(baseParentName)) {
            throw InheritanceException(
                "Class '" + className + "' cannot extend interface '" + parentName + "'. "
                "Classes can only extend other classes. Use 'implements' to implement interfaces.",
                className,
                parentName,
                location);
        }
    }

    void InheritanceValidator::validateInterfaceCannotExtendClass(
        const std::string& interfaceName,
        const std::string& parentName,
        const SourceLocation& location,
        std::shared_ptr<EvaluationContext> context)
    {
        if (!context) {
            throw InheritanceException("No evaluation context available for validation", location);
        }

        auto env = context->getEnvironment();

        // Extract base parent name from generic type (e.g., "Container<T>" -> "Container")
        std::string baseParentName = parentName;
        size_t genericStart = parentName.find('<');
        if (genericStart != std::string::npos) {
            baseParentName = parentName.substr(0, genericStart);
        }

        // Check if the parent is actually a class
        if (env->findClass(baseParentName)) {
            throw InheritanceException(
                "Interface '" + interfaceName + "' cannot extend class '" + parentName + "'. "
                "Interfaces can only extend other interfaces.",
                interfaceName,
                parentName,
                location);
        }
    }

} // namespace validation
} // namespace evaluator
