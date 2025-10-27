#include "AnnotationValidator.hpp"
#include "../../errors/InheritanceException.hpp"
#include "../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../../runtimeTypes/klass/InterfaceRegistry.hpp"
#include <sstream>

namespace evaluator::validation
{
    void AnnotationValidator::validateClassAnnotations(
        std::shared_ptr<ClassDefinition> classDefinition,
        std::shared_ptr<Environment> environment)
    {
        if (!classDefinition || !environment)
        {
            return;
        }

        // Get parent class if exists
        std::shared_ptr<ClassDefinition> parentClass = nullptr;
        if (classDefinition->hasParentClass())
        {
            auto classRegistry = environment->getClassRegistry();
            if (classRegistry)
            {
                parentClass = classRegistry->findClass(classDefinition->getParentClassName());
            }
        }

        // Validate annotations on all instance methods
        for (const auto& [methodName, methodDef] : classDefinition->getInstanceMethods())
        {
            if (methodDef->hasAnnotation("Override"))
            {
                validateOverrideAnnotation(
                    methodDef.get(),
                    classDefinition,
                    environment,
                    methodDef->getSourceLocation()  // Use stored source location for accurate error reporting
                );
            }
        }
    }

    void AnnotationValidator::validateOverrideAnnotation(
        const MethodDefinition* method,
        std::shared_ptr<ClassDefinition> containingClass,
        std::shared_ptr<Environment> environment,
        const SourceLocation& location)
    {
        if (!method || !containingClass || !environment)
        {
            return;
        }

        bool foundMatchingMethod = false;

        // Check parent class hierarchy
        if (containingClass->hasParentClass())
        {
            auto classRegistry = environment->getClassRegistry();
            if (classRegistry)
            {
                auto parentClass = classRegistry->findClass(containingClass->getParentClassName());
                if (parentClass)
                {
                    auto parentMethod = findMatchingMethodInParent(method, parentClass);
                    if (parentMethod)
                    {
                        foundMatchingMethod = true;
                    }
                }
            }
        }

        // If not found in parent, check interfaces
        if (!foundMatchingMethod)
        {
            foundMatchingMethod = findMatchingMethodInInterfaces(
                method,
                containingClass,
                environment
            );
        }

        // If still not found, throw error
        if (!foundMatchingMethod)
        {
            std::string errorMsg = generateOverrideErrorMessage(
                method->getName(),
                containingClass->getName()
            );
            throw InheritanceException(errorMsg, location);
        }
    }

    std::shared_ptr<MethodDefinition> AnnotationValidator::findMatchingMethodInParent(
        const MethodDefinition* method,
        std::shared_ptr<ClassDefinition> parentClass)
    {
        if (!method || !parentClass)
        {
            return nullptr;
        }

        // Search in parent class hierarchy
        while (parentClass)
        {
            // Check instance methods
            const auto& instanceMethods = parentClass->getInstanceMethods();
            auto it = instanceMethods.find(method->getName());
            if (it != instanceMethods.end())
            {
                if (methodSignaturesMatch(method, it->second.get()))
                {
                    return it->second;
                }
            }

            // Move to next parent in hierarchy
            parentClass = parentClass->getParentClass();
        }

        return nullptr;
    }

    bool AnnotationValidator::findMatchingMethodInInterfaces(
        const MethodDefinition* method,
        std::shared_ptr<ClassDefinition> containingClass,
        std::shared_ptr<Environment> environment)
    {
        if (!method || !containingClass || !environment)
        {
            return false;
        }

        auto interfaceRegistry = environment->getInterfaceRegistry();
        if (!interfaceRegistry)
        {
            return false;
        }

        // Get the parameter count from the method
        size_t methodParamCount = method->getParameters().size();

        // Check all implemented interfaces
        for (const std::string& interfaceName : containingClass->getImplementedInterfaces())
        {
            auto interfaceDef = interfaceRegistry->findInterface(interfaceName);
            if (interfaceDef)
            {
                // Check if interface has a method with matching signature
                if (interfaceDef->hasMethod(method->getName()))
                {
                    // findMethod requires methodName and paramCount
                    auto interfaceMethod = interfaceDef->findMethod(method->getName(), methodParamCount);
                    if (interfaceMethod)
                    {
                        // Found a matching method in the interface
                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool AnnotationValidator::methodSignaturesMatch(
        const MethodDefinition* method1,
        const MethodDefinition* method2)
    {
        if (!method1 || !method2)
        {
            return false;
        }

        // Check method names
        if (method1->getName() != method2->getName())
        {
            return false;
        }

        // Get parameters from both methods
        const auto& params1 = method1->getParameters();
        const auto& params2 = method2->getParameters();

        // Check parameter counts
        if (params1.size() != params2.size())
        {
            return false;
        }

        // Check parameter types
        for (size_t i = 0; i < params1.size(); ++i)
        {
            // Compare parameter types (using operator== from ParameterType)
            if (!(params1[i].second == params2[i].second))
            {
                return false;
            }
        }

        // Check return types
        if (method1->getReturnType() != method2->getReturnType())
        {
            return false;
        }

        return true;
    }

    std::string AnnotationValidator::generateOverrideErrorMessage(
        const std::string& methodName,
        const std::string& className)
    {
        std::ostringstream oss;
        oss << "Method '" << methodName << "' in class '" << className
            << "' is marked with @Override but does not override any method from a parent class or interface.\n\n"
            << "Possible causes:\n"
            << "  - The method name may be misspelled\n"
            << "  - The parameter types may not match the parent method\n"
            << "  - The parent class does not have this method\n"
            << "  - The class does not implement an interface with this method\n\n"
            << "Please verify the method signature matches exactly with the parent method or interface.";
        return oss.str();
    }
}
