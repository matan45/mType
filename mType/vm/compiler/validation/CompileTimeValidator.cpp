#include "CompileTimeValidator.hpp"
#include <cstddef>
#include "../../../errors/TypeException.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"
#include "../../../types/TypeConversionUtils.hpp"

namespace vm::compiler::validation
{
    CompileTimeValidator::CompileTimeValidator(
        std::shared_ptr<environment::Environment> env,
        bytecode::BytecodeProgram& prog
    )
        : environment(env)
        , program(prog)
    {
    }

    void CompileTimeValidator::validateFunctionExists(const std::string& functionName,
                                                     const ast::SourceLocation& location,
                                                     const std::string& currentClassName)
    {
        // For overload-aware validation, check if ANY function with this name exists
        // The exact overload will be resolved during compilation

        // Check if any function with this name is registered (check FunctionRegistry for overloads)
        auto funcRegistry = environment->getFunctionRegistry();
        if (funcRegistry)
        {
            auto overloads = funcRegistry->getAllFunctionOverloads(functionName);
            if (!overloads.empty())
            {
                return; // Found at least one overload
            }
        }

        // Check if function is registered in the program (includes pre-registered functions)
        if (program.getFunction(functionName))
        {
            return; // Found
        }

        // Check if it's a native function
        auto nativeRegistry = environment->getNativeRegistry();
        if (nativeRegistry && nativeRegistry->hasNativeFunction(functionName))
        {
            return; // Found as native function
        }

        // If we're in a class context, check if it's a static method of the current class
        if (!currentClassName.empty())
        {
            auto classRegistry = environment->getClassRegistry();
            auto classDef = classRegistry->findClass(currentClassName);
            if (classDef)
            {
                // Check if ANY static method with this name exists (overload-aware)
                const auto& staticMethods = classDef->getStaticMethods();
                if (staticMethods.find(functionName) != staticMethods.end())
                {
                    return; // Found as static method of current class
                }
            }
        }

        // MYT-289 follow-up: explicit opt-out for runtime-resolved natives.
        // Names with the `__native__` prefix bypass the compile-time
        // existence check — they're registered at runtime by a plugin
        // (`__plugin_load`) so the check would otherwise reject every
        // plugin call site. Built-in natives keep the regular `__name`
        // convention and remain compile-time validated, so typos like
        // `__json_serialze` still fail at compile time. The bypass costs
        // typo detection for plugin-bound names — accepted trade for
        // unblocking dynamic FFI.
        constexpr const char* kRuntimeNativePrefix = "__native__";
        constexpr size_t kRuntimeNativePrefixLen = 10;
        if (functionName.size() >= kRuntimeNativePrefixLen
            && functionName.compare(0, kRuntimeNativePrefixLen, kRuntimeNativePrefix) == 0)
        {
            return; // Treat as runtime-resolved plugin native
        }

        throw errors::TypeException(
            "Function '" + functionName + "' not found. Did you forget to declare it?",
            location
        );
    }

    void CompileTimeValidator::validateStaticMethodExists(const std::string& className,
                                                         const std::string& methodName,
                                                         size_t argCount,
                                                         const ast::SourceLocation& location)
    {
        // Strip nullable suffix '?' for class lookup
        std::string baseName = ::types::TypeConversionUtils::stripNullable(className);

        // Skip validation for generic type parameters (T, K, V, E, etc.)
        // Though static methods on generic types are unusual, we handle it consistently
        if (baseName.length() == 1 && std::isupper(baseName[0]))
        {
            return; // Generic type parameter - skip validation
        }

        // Validate class exists first
        validateClassExists(baseName, location);

        // Get class definition
        auto classRegistry = environment->getClassRegistry();
        auto classDef = classRegistry->findClass(baseName);
        if (!classDef)
        {
            throw errors::TypeException(
                "Class '" + baseName + "' not found for static method call",
                location
            );
        }

        // For overload-aware validation, check if ANY static method with this name exists
        // The exact overload will be resolved during compilation based on argument types
        auto overloads = classDef->getAllStaticMethodOverloads(methodName);
        if (!overloads.empty())
        {
            return; // Found at least one overload
        }

        // Check parent classes for inherited static methods
        auto currentClass = classDef;
        while (currentClass && currentClass->hasParentClass())
        {
            auto parentClass = classRegistry->findClass(currentClass->getParentClassName());
            if (parentClass)
            {
                auto parentOverloads = parentClass->getAllStaticMethodOverloads(methodName);
                if (!parentOverloads.empty())
                {
                    return; // Found in parent class
                }
                currentClass = parentClass;
            }
            else
            {
                break;
            }
        }

        throw errors::TypeException(
            "Static method '" + methodName + "' not found in class '" + baseName + "' or its parent classes",
            location
        );
    }

    void CompileTimeValidator::validateInstanceMethodExists(const std::string& className,
                                                           const std::string& methodName,
                                                           size_t argCount,
                                                           const ast::SourceLocation& location)
    {
        // Strip nullable suffix '?' for class lookup
        std::string baseName = ::types::TypeConversionUtils::stripNullable(className);

        // Skip validation for generic type parameters (T, K, V, E, etc.)
        // Generic types will be validated at runtime when instantiated
        if (baseName.length() == 1 && std::isupper(baseName[0]))
        {
            return; // Generic type parameter - skip validation
        }

        // Validate class/interface exists first
        validateClassExists(baseName, location);

        // Check if it's a class
        auto classRegistry = environment->getClassRegistry();
        auto classDef = classRegistry->findClass(baseName);
        if (classDef)
        {
            // For overload-aware validation, check if ANY instance method with this name exists
            // The exact overload will be resolved during compilation based on argument types
            auto overloads = classDef->getAllInstanceMethodOverloads(methodName);
            if (!overloads.empty())
            {
                return; // Found at least one overload
            }

            // Check parent classes for inherited methods
            auto currentClass = classDef;
            while (currentClass && currentClass->hasParentClass())
            {
                std::string parentName = currentClass->getParentClassName();

                // Extract base class name (strip generic parameters like <T>)
                // E.g., "Container<T>" -> "Container"
                std::string baseParentName = parentName;
                size_t genericStart = parentName.find('<');
                if (genericStart != std::string::npos) {
                    baseParentName = parentName.substr(0, genericStart);
                }

                auto parentClass = classRegistry->findClass(baseParentName);
                if (parentClass)
                {
                    auto parentOverloads = parentClass->getAllInstanceMethodOverloads(methodName);
                    if (!parentOverloads.empty())
                    {
                        return; // Found in parent class
                    }
                    currentClass = parentClass;
                }
                else
                {
                    break;
                }
            }

            throw errors::TypeException(
                "Instance method '" + methodName + "' not found in class '" + baseName + "' or its parent classes",
                location
            );
        }

        // Check if it's an interface
        auto interfaceDef = environment->findInterface(baseName);
        if (interfaceDef)
        {
            // For interfaces, check if the method exists in this interface or any parent interfaces
            if (!hasMethodInInterfaceHierarchy(interfaceDef, methodName))
            {
                throw errors::TypeException(
                    "Method '" + methodName + "' not found in interface '" + baseName + "' or its parent interfaces",
                    location
                );
            }
            return;
        }

        // This should never happen since validateClassExists would have thrown
        throw errors::TypeException(
            "Type '" + baseName + "' not found for instance method call",
            location
        );
    }

    void CompileTimeValidator::validateConstructorExists(const std::string& className,
                                                        size_t argCount,
                                                        const ast::SourceLocation& location)
    {
        // Strip nullable suffix '?' for class lookup
        std::string baseName = ::types::TypeConversionUtils::stripNullable(className);

        // Skip validation for generic type parameters (T, K, V, E, etc.)
        // Generic types will be validated at runtime when instantiated
        if (baseName.length() == 1 && std::isupper(baseName[0]))
        {
            return; // Generic type parameter - skip validation
        }

        // Validate class exists first
        validateClassExists(baseName, location);

        // Get class definition
        auto classRegistry = environment->getClassRegistry();
        auto classDef = classRegistry->findClass(baseName);
        if (!classDef)
        {
            throw errors::TypeException(
                "Class '" + baseName + "' not found for constructor call",
                location
            );
        }

        // Check if constructor exists
        auto constructor = classDef->findConstructor(argCount);
        if (!constructor)
        {
            // Special case: if looking for 0-argument constructor and class has no constructors,
            // allow it (implicit default constructor)
            if (argCount == 0 && classDef->getConstructors().empty())
            {
                return; // Implicit default constructor
            }

            throw errors::TypeException(
                "no matching overload for call to constructor of class '" + baseName
                + "' with " + std::to_string(argCount) + " argument(s)",
                location
            );
        }
    }

    void CompileTimeValidator::validateVariableExists(const std::string& varName,
                                                     const ast::SourceLocation& location)
    {
        // Stub: needs scope information from VariableTracker.
    }

    void CompileTimeValidator::validateClassExists(const std::string& className,
                                                  const ast::SourceLocation& location)
    {
        // Strip nullable suffix '?' - e.g. "Address?" -> "Address"
        std::string baseName = ::types::TypeConversionUtils::stripNullable(className);

        // Skip validation for generic type parameters (T, K, V, E, etc.)
        // Generic type parameters are typically single uppercase letters
        if (baseName.length() == 1 && std::isupper(baseName[0]))
        {
            return; // Assume it's a generic type parameter
        }

        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry)
        {
            throw errors::TypeException(
                "Class registry not available during validation",
                location
            );
        }

        // Check if it's a class
        auto classDef = classRegistry->findClass(baseName);
        if (classDef)
        {
            return; // Found as class
        }

        // Check if it's an interface
        auto interfaceDef = environment->findInterface(baseName);
        if (interfaceDef)
        {
            return; // Found as interface
        }

        // Not found as class or interface
        throw errors::TypeException(
            "Class or interface '" + baseName + "' not found. Did you forget to import it?",
            location
        );
    }

    void CompileTimeValidator::validateParentClassExists(const std::string& parentClassName,
                                                        const ast::SourceLocation& location)
    {
        validateClassExists(parentClassName, location);
    }

    std::string CompileTimeValidator::getQualifiedMethodName(const std::string& className,
                                                            const std::string& methodName,
                                                            size_t argCount,
                                                            bool isStatic)
    {
        if (isStatic)
        {
            return className + "::" + methodName;
        }
        return className + "::" + methodName + "/" + std::to_string(argCount);
    }

    bool CompileTimeValidator::hasMethodInInterfaceHierarchy(
        std::shared_ptr<runtimeTypes::klass::InterfaceDefinition> interfaceDef,
        const std::string& methodName)
    {
        if (!interfaceDef)
        {
            return false;
        }

        // Check if the method exists in the current interface
        if (interfaceDef->hasMethod(methodName))
        {
            return true;
        }

        // Recursively check parent interfaces
        for (const auto& parentInterfaceName : interfaceDef->getExtendedInterfaces())
        {
            auto parentInterface = environment->findInterface(parentInterfaceName);
            if (parentInterface && hasMethodInInterfaceHierarchy(parentInterface, methodName))
            {
                return true;
            }
        }

        return false;
    }
}
