#include "AnnotationValidator.hpp"
#include "../errors/InheritanceException.hpp"
#include "../errors/TypeException.hpp"
#include "../runtimeTypes/klass/InterfaceDefinition.hpp"
#include "../runtimeTypes/klass/InterfaceRegistry.hpp"
#include "../value/ValueType.hpp"
#include "../constants/ExceptionConstants.hpp"
#include <sstream>
#include <unordered_set>

namespace validation
{
    void AnnotationValidator::validateClassAnnotations(
        std::shared_ptr<ClassDefinition> classDefinition,
        std::shared_ptr<Environment> environment)
    {
        if (!classDefinition || !environment)
        {
            return;
        }

        // Validate @Script annotation on class
        if (auto scriptAnnotation = classDefinition->getAnnotation("Script"))
        {
            SourceLocation location = scriptAnnotation->getLocation();
            validateScriptAnnotation(classDefinition, location);
        }

        // Validate @EntryPoint annotation on class
        if (auto entryPointAnnotation = classDefinition->getAnnotation("EntryPoint"))
        {
            SourceLocation location = entryPointAnnotation->getLocation();
            validateEntryPointAnnotation(classDefinition, location);
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
        for (const auto& [methodName, methodOverloads] : classDefinition->getInstanceMethods())
        {
            // Iterate through all overloads
            for (const auto& methodDef : methodOverloads)
            {
                if (methodDef->hasAnnotation("Override"))
                {
                    validateOverrideAnnotation(
                        methodDef.get(),
                        classDefinition,
                        environment,
                        methodDef->getSourceLocation() // Use stored source location for accurate error reporting
                    );
                }

                // Validate @Throw annotation if present
                if (auto throwAnnotation = methodDef->getAnnotation("Throw"))
                {
                    validateThrowAnnotation(
                        throwAnnotation,
                        environment,
                        methodDef->getSourceLocation()
                    );
                }
            }
        }

        // Validate annotations on all static methods
        for (const auto& [methodName, methodOverloads] : classDefinition->getStaticMethods())
        {
            // Iterate through all overloads
            for (const auto& methodDef : methodOverloads)
            {
                // Validate @Throw annotation if present
                if (auto throwAnnotation = methodDef->getAnnotation("Throw"))
                {
                    validateThrowAnnotation(
                        throwAnnotation,
                        environment,
                        methodDef->getSourceLocation()
                    );
                }
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
            std::string fullParentName = containingClass->getParentClassName();

            // Extract base class name (strip generic type parameters if present)
            // E.g., "Container<DataItem>" -> "Container"
            std::string baseParentName = fullParentName;
            size_t genericStart = fullParentName.find('<');
            if (genericStart != std::string::npos)
            {
                baseParentName = fullParentName.substr(0, genericStart);
            }

            auto classRegistry = environment->getClassRegistry();
            if (classRegistry)
            {
                auto parentClass = classRegistry->findClass(baseParentName);
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
                // Check all overloads for matching signature
                for (const auto& parentMethod : it->second)
                {
                    if (methodSignaturesMatch(method, parentMethod.get()))
                    {
                        return parentMethod;
                    }
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
        // For instance methods, exclude the implicit 'this' parameter when comparing with interface methods
        size_t methodParamCount = method->getParameters().size();
        if (!method->isStatic() && methodParamCount > 0)
        {
            methodParamCount--; // Exclude 'this' parameter
        }

        // Check all implemented interfaces
        for (const std::string& interfaceName : containingClass->getImplementedInterfaces())
        {
            auto interfaceDef = interfaceRegistry->findInterface(interfaceName);
            if (interfaceDef)
            {
                // Check if interface has a method with matching signature
                // Optimized: Use findMethod directly instead of hasMethod + findMethod
                // This reduces from 2 lookups to 1 lookup (both iterate methodSignatures)
                auto interfaceMethod = interfaceDef->findMethod(method->getName(), methodParamCount);
                if (interfaceMethod)
                {
                    // Found a matching method in the interface
                    return true;
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
        // For instance methods, skip index 0 ('this' parameter) as it will differ between parent and child
        size_t startIndex = (!method1->isStatic() && !params1.empty()) ? 1 : 0;
        for (size_t i = startIndex; i < params1.size(); ++i)
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

    void AnnotationValidator::validateScriptAnnotation(
        std::shared_ptr<ClassDefinition> classDefinition,
        const SourceLocation& location)
    {
        if (!classDefinition)
        {
            return;
        }

        const std::string& className = classDefinition->getName();

        // Check 1: Class must not be abstract
        if (classDefinition->isAbstract())
        {
            std::ostringstream oss;
            oss << "Class '" << className << "' is marked with @Script but is abstract.\n\n"
                << "@Script classes must be concrete classes that can be instantiated.\n"
                << "Please remove the 'abstract' modifier or the @Script annotation.";
            throw TypeException(oss.str(), location);
        }

        // Check 2: Must have default constructor (0 parameters)
        bool hasDefaultConstructor = false;
        const auto& constructors = classDefinition->getConstructors();

        for (const auto& constructor : constructors)
        {
            if (constructor->getParameters().empty())
            {
                hasDefaultConstructor = true;
                break;
            }
        }

        if (!hasDefaultConstructor)
        {
            std::ostringstream oss;
            oss << "Class '" << className << "' is marked with @Script but does not have a default constructor.\n\n"
                << "@Script classes must have a constructor with no parameters.\n"
                << "Please add a default constructor:\n"
                << "  constructor() {\n"
                << "    // initialization code\n"
                << "  }";
            throw TypeException(oss.str(), location);
        }

        // Check 3: Must have onUpdate(float): void method
        bool hasUpdateMethod = false;
        const auto& instanceMethods = classDefinition->getInstanceMethods();

        auto it = instanceMethods.find("onUpdate");
        if (it != instanceMethods.end())
        {
            // Check all overloads for the required signature
            for (const auto& method : it->second)
            {
                const auto& params = method->getParameters();

                // Check: exactly 1 real parameter of type float (plus implicit 'this'), return type void
                // Instance methods have 'this' as first parameter, so size should be 2 total
                if (params.size() == 2 &&
                    params[1].second.basicType == value::ValueType::FLOAT &&
                    method->getReturnType() == value::ValueType::VOID)
                {
                    hasUpdateMethod = true;
                    break;
                }
            }
        }

        if (!hasUpdateMethod)
        {
            std::ostringstream oss;
            oss << "Class '" << className <<
                "' is marked with @Script but does not have the required onUpdate method.\n\n"
                << "@Script classes must have an onUpdate method with signature:\n"
                << "  function onUpdate(float deltaTime): void\n\n"
                << "Please add this method to your class.";
            throw TypeException(oss.str(), location);
        }

        // Check 4: Must have onStart(): void method
        bool hasStartMethod = false;

        auto startIt = instanceMethods.find("onStart");
        if (startIt != instanceMethods.end())
        {
            for (const auto& method : startIt->second)
            {
                const auto& params = method->getParameters();

                // Check: no parameters (only implicit 'this'), return type void
                if (params.size() == 1 &&
                    method->getReturnType() == value::ValueType::VOID)
                {
                    hasStartMethod = true;
                    break;
                }
            }
        }

        if (!hasStartMethod)
        {
            std::ostringstream oss;
            oss << "Class '" << className <<
                "' is marked with @Script but does not have the required onStart method.\n\n"
                << "@Script classes must have an onStart method with signature:\n"
                << "  function onStart(): void\n\n"
                << "Please add this method to your class.";
            throw TypeException(oss.str(), location);
        }

        // Check 5: Must have onDestroy(): void method
        bool hasDestroyMethod = false;

        auto destroyIt = instanceMethods.find("onDestroy");
        if (destroyIt != instanceMethods.end())
        {
            for (const auto& method : destroyIt->second)
            {
                const auto& params = method->getParameters();

                // Check: no parameters (only implicit 'this'), return type void
                if (params.size() == 1 &&
                    method->getReturnType() == value::ValueType::VOID)
                {
                    hasDestroyMethod = true;
                    break;
                }
            }
        }

        if (!hasDestroyMethod)
        {
            std::ostringstream oss;
            oss << "Class '" << className <<
                "' is marked with @Script but does not have the required onDestroy method.\n\n"
                << "@Script classes must have an onDestroy method with signature:\n"
                << "  function onDestroy(): void\n\n"
                << "Please add this method to your class.";
            throw TypeException(oss.str(), location);
        }
    }

    void AnnotationValidator::validateEntryPointAnnotation(
        std::shared_ptr<ClassDefinition> classDefinition,
        const SourceLocation& location)
    {
        if (!classDefinition)
        {
            return;
        }

        const std::string& className = classDefinition->getName();

        // Check 1: Class must not be abstract
        if (classDefinition->isAbstract())
        {
            std::ostringstream oss;
            oss << "Class '" << className << "' is marked with @EntryPoint but is abstract.\n\n"
                << "@EntryPoint classes must be concrete classes.\n"
                << "Please remove the 'abstract' modifier or the @EntryPoint annotation.";
            throw TypeException(oss.str(), location);
        }

        // Check 2: Must have a static main method
        bool hasMainMethod = false;
        const auto& staticMethods = classDefinition->getStaticMethods();

        auto it = staticMethods.find("main");
        if (it != staticMethods.end())
        {
            for (const auto& method : it->second)
            {
                const auto& params = method->getParameters();

                // Static methods do NOT have implicit 'this', so exactly 1 parameter (args)
                // Return type must be void
                if (params.size() == 1 &&
                    params[0].second.basicType == value::ValueType::ARRAY &&
                    method->getReturnType() == value::ValueType::VOID)
                {
                    hasMainMethod = true;
                    break;
                }
            }
        }

        if (!hasMainMethod)
        {
            std::ostringstream oss;
            oss << "Class '" << className
                << "' is marked with @EntryPoint but does not have the required main method.\n\n"
                << "@EntryPoint classes must have a static main method with signature:\n"
                << "  static function main(String[] args): void\n\n"
                << "Please add this method to your class.";
            throw TypeException(oss.str(), location);
        }
    }

    void AnnotationValidator::validateThrowAnnotation(
        std::shared_ptr<ast::nodes::annotations::AnnotationNode> throwAnnotation,
        std::shared_ptr<Environment> environment,
        const SourceLocation& location)
    {
        if (!throwAnnotation)
        {
            return;
        }

        // Get the exceptions parameter from the annotation
        std::string exceptionsParam = throwAnnotation->getParameter("exceptions");
        if (exceptionsParam.empty())
        {
            throw TypeException(
                "@Throw annotation must specify at least one exception class",
                location
            );
        }

        // Parse the comma-separated exception list
        std::vector<std::string> exceptionNames = parseExceptionList(exceptionsParam);

        // Check for duplicates
        checkForDuplicates(exceptionNames, location);

        // Validate each exception class
        auto classRegistry = environment->getClassRegistry();
        for (const auto& exceptionName : exceptionNames)
        {
            // Check if the class exists
            auto exceptionClass = classRegistry->findClass(exceptionName);
            if (!exceptionClass)
            {
                std::ostringstream oss;
                oss << "Exception class '" << exceptionName << "' in @Throw annotation does not exist.\n\n"
                    << "Make sure the exception class is defined and imported if necessary.";
                throw TypeException(oss.str(), location);
            }

            // Check if the class inherits from Exception
            if (!isExceptionClass(exceptionName, environment))
            {
                std::ostringstream oss;
                oss << "Class '" << exceptionName << "' in @Throw annotation must extend "
                    << constants::exception::BASE_EXCEPTION_CLASS << ".\n\n"
                    << "Exception classes must inherit from the "
                    << constants::exception::BASE_EXCEPTION_CLASS << " base class.";
                throw TypeException(oss.str(), location);
            }
        }
    }

    // Helper function to trim whitespace from both ends of a string
    static std::string trim(const std::string& str)
    {
        const std::string whitespace = " \t\n\r";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos)
        {
            return ""; // String is all whitespace
        }
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    std::vector<std::string> AnnotationValidator::parseExceptionList(const std::string& exceptionsParam)
    {
        std::vector<std::string> exceptionNames;
        std::stringstream ss(exceptionsParam);
        std::string token;

        // Use std::getline with comma delimiter
        while (std::getline(ss, token, ','))
        {
            std::string trimmed = trim(token);
            if (!trimmed.empty())
            {
                exceptionNames.push_back(trimmed);
            }
        }

        return exceptionNames;
    }

    bool AnnotationValidator::isExceptionClass(
        const std::string& className,
        std::shared_ptr<Environment> environment)
    {
        auto classRegistry = environment->getClassRegistry();
        auto classDefinition = classRegistry->findClass(className);

        if (!classDefinition)
        {
            return false;
        }

        // Check if the class is named "Exception" (base exception class)
        if (className == constants::exception::BASE_EXCEPTION_CLASS)
        {
            return true;
        }

        // Check if the class inherits from Exception
        // Walk up the inheritance hierarchy
        std::string currentClassName = className;
        while (true)
        {
            auto currentClass = classRegistry->findClass(currentClassName);
            if (!currentClass)
            {
                break;
            }

            if (!currentClass->hasParentClass())
            {
                break;
            }

            std::string parentClassName = currentClass->getParentClassName();
            if (parentClassName == constants::exception::BASE_EXCEPTION_CLASS)
            {
                return true;
            }

            currentClassName = parentClassName;
        }

        return false;
    }

    void AnnotationValidator::checkForDuplicates(
        const std::vector<std::string>& exceptionNames,
        const SourceLocation& location)
    {
        std::unordered_set<std::string> seen;
        for (const auto& name : exceptionNames)
        {
            if (seen.find(name) != seen.end())
            {
                // Found a duplicate - this could be a warning instead of an error
                // For now, we'll just ignore duplicates silently
                // In the future, this could log a warning
                continue;
            }
            seen.insert(name);
        }
    }
}
