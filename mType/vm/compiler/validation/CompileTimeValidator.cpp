#include "CompileTimeValidator.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"

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
                                                     const ast::SourceLocation& location)
    {
        // Check if function is registered in the program (includes pre-registered functions)
        if (!program.getFunction(functionName))
        {
            // Check if it's a native function
            auto nativeRegistry = environment->getNativeRegistry();
            if (!nativeRegistry || !nativeRegistry->hasNativeFunction(functionName))
            {
                throw errors::TypeException(
                    "Function '" + functionName + "' not found. Did you forget to declare it?",
                    location
                );
            }
        }
    }

    void CompileTimeValidator::validateStaticMethodExists(const std::string& className,
                                                         const std::string& methodName,
                                                         size_t argCount,
                                                         const ast::SourceLocation& location)
    {
        // Skip validation for generic type parameters (T, K, V, E, etc.)
        // Though static methods on generic types are unusual, we handle it consistently
        if (className.length() == 1 && std::isupper(className[0]))
        {
            return; // Generic type parameter - skip validation
        }

        // Validate class exists first
        validateClassExists(className, location);

        // Get class definition
        auto classRegistry = environment->getClassRegistry();
        auto classDef = classRegistry->findClass(className);
        if (!classDef)
        {
            throw errors::TypeException(
                "Class '" + className + "' not found for static method call",
                location
            );
        }

        // Check if static method exists with the correct argument count (including inherited)
        auto staticMethod = classDef->findStaticMethodInHierarchy(methodName, argCount);
        if (!staticMethod)
        {
            throw errors::TypeException(
                "Static method '" + methodName + "' with " + std::to_string(argCount) +
                " argument(s) not found in class '" + className + "' or its parent classes",
                location
            );
        }
    }

    void CompileTimeValidator::validateInstanceMethodExists(const std::string& className,
                                                           const std::string& methodName,
                                                           size_t argCount,
                                                           const ast::SourceLocation& location)
    {
        // Skip validation for generic type parameters (T, K, V, E, etc.)
        // Generic types will be validated at runtime when instantiated
        if (className.length() == 1 && std::isupper(className[0]))
        {
            return; // Generic type parameter - skip validation
        }

        // Validate class/interface exists first
        validateClassExists(className, location);

        // Check if it's a class
        auto classRegistry = environment->getClassRegistry();
        auto classDef = classRegistry->findClass(className);
        if (classDef)
        {
            // Check if instance method exists (including inherited methods)
            auto method = classDef->findMethodInHierarchy(methodName, argCount);
            if (!method)
            {
                throw errors::TypeException(
                    "Instance method '" + methodName + "' with " + std::to_string(argCount) +
                    " parameters not found in class '" + className + "' or its parent classes",
                    location
                );
            }
            return;
        }

        // Check if it's an interface
        auto interfaceDef = environment->findInterface(className);
        if (interfaceDef)
        {
            // For interfaces, we just check if the method exists (interfaces don't track param counts)
            if (!interfaceDef->hasMethod(methodName))
            {
                throw errors::TypeException(
                    "Method '" + methodName + "' not found in interface '" + className + "'",
                    location
                );
            }
            return;
        }

        // This should never happen since validateClassExists would have thrown
        throw errors::TypeException(
            "Type '" + className + "' not found for instance method call",
            location
        );
    }

    void CompileTimeValidator::validateConstructorExists(const std::string& className,
                                                        size_t argCount,
                                                        const ast::SourceLocation& location)
    {
        // Skip validation for generic type parameters (T, K, V, E, etc.)
        // Generic types will be validated at runtime when instantiated
        if (className.length() == 1 && std::isupper(className[0]))
        {
            return; // Generic type parameter - skip validation
        }

        // Validate class exists first
        validateClassExists(className, location);

        // Get class definition
        auto classRegistry = environment->getClassRegistry();
        auto classDef = classRegistry->findClass(className);
        if (!classDef)
        {
            throw errors::TypeException(
                "Class '" + className + "' not found for constructor call",
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
                "Constructor for class '" + className + "' with " + std::to_string(argCount) +
                " parameters not found",
                location
            );
        }
    }

    void CompileTimeValidator::validateVariableExists(const std::string& varName,
                                                     const ast::SourceLocation& location)
    {
        // This is a simplified check - in practice, you'd need scope information
        // For now, we'll skip this as it requires more context about current scope
        // This should be integrated with VariableTracker in the future
    }

    void CompileTimeValidator::validateClassExists(const std::string& className,
                                                  const ast::SourceLocation& location)
    {
        // Skip validation for generic type parameters (T, K, V, E, etc.)
        // Generic type parameters are typically single uppercase letters
        if (className.length() == 1 && std::isupper(className[0]))
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
        auto classDef = classRegistry->findClass(className);
        if (classDef)
        {
            return; // Found as class
        }

        // Check if it's an interface
        auto interfaceDef = environment->findInterface(className);
        if (interfaceDef)
        {
            return; // Found as interface
        }

        // Not found as class or interface
        throw errors::TypeException(
            "Class or interface '" + className + "' not found. Did you forget to import it?",
            location
        );
    }

    void CompileTimeValidator::validateParentClassExists(const std::string& parentClassName,
                                                        const ast::SourceLocation& location)
    {
        validateClassExists(parentClassName, location);
    }

    void CompileTimeValidator::validateTypeIsNotRawGeneric(const std::string& typeName,
                                                          const ast::SourceLocation& location)
    {
        // Skip validation for:
        // 1. Generic type parameters (T, K, V, E, etc.) - single uppercase letters
        // 2. Primitive types (int, float, string, bool, void)
        // 3. Type names that already contain generic arguments (e.g., "Wrapper<Int>")

        if (typeName.length() == 1 && std::isupper(typeName[0]))
        {
            return; // Generic type parameter
        }

        // Check if type name already contains generic arguments
        if (typeName.find('<') != std::string::npos)
        {
            return; // Already has type arguments
        }

        // Check if it's a primitive type
        if (typeName == "int" || typeName == "float" || typeName == "string" ||
            typeName == "bool" || typeName == "void" || typeName == "object")
        {
            return; // Primitive type - not generic
        }

        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry)
        {
            return; // No class registry available - skip validation
        }

        // Check if this is a generic class
        auto classDef = classRegistry->findClass(typeName);
        if (classDef && !classDef->getGenericParameters().empty())
        {
            // This is a generic class but no type arguments were provided
            std::string paramList;
            const auto& params = classDef->getGenericParameters();
            for (size_t i = 0; i < params.size(); ++i)
            {
                if (i > 0) paramList += ", ";
                paramList += params[i].name;
            }

            throw errors::TypeException(
                "Generic type '" + typeName + "' requires type arguments. " +
                "Use '" + typeName + "<" + paramList + ">' instead of raw type '" + typeName + "'",
                location
            );
        }

        // Also check interfaces
        auto interfaceDef = environment->findInterface(typeName);
        if (interfaceDef && !interfaceDef->getGenericParameters().empty())
        {
            // This is a generic interface but no type arguments were provided
            std::string paramList;
            const auto& params = interfaceDef->getGenericParameters();
            for (size_t i = 0; i < params.size(); ++i)
            {
                if (i > 0) paramList += ", ";
                paramList += params[i].name;
            }

            throw errors::TypeException(
                "Generic interface '" + typeName + "' requires type arguments. " +
                "Use '" + typeName + "<" + paramList + ">' instead of raw type '" + typeName + "'",
                location
            );
        }
    }

    void CompileTimeValidator::validateAllMethodsHaveBytecode(const std::string& className,
                                                             const ast::SourceLocation& location)
    {
        // Get class definition
        auto classRegistry = environment->getClassRegistry();
        auto classDef = classRegistry->findClass(className);
        if (!classDef)
        {
            return; // Class doesn't exist - will be caught elsewhere
        }

        // Validate all instance methods have bytecode
        const auto& methods = classDef->getInstanceMethods();
        for (const auto& pair : methods)
        {
            std::string qualifiedName = className + "::" + pair.first;
            if (!program.getFunction(qualifiedName))
            {
                throw errors::TypeException(
                    "Instance method '" + className + "::" + pair.first +
                    "' declared but not implemented. All instance methods must have bytecode implementation.",
                    location
                );
            }
        }

        // Validate all static methods have bytecode
        const auto& staticMethods = classDef->getStaticMethods();
        for (const auto& pair : staticMethods)
        {
            std::string qualifiedName = className + "::" + pair.first + "$static";
            if (!program.getFunction(qualifiedName))
            {
                throw errors::TypeException(
                    "Static method '" + className + "::" + pair.first + "$static" +
                    "' declared but not implemented. All static methods must have bytecode implementation.",
                    location
                );
            }
        }

        // Validate all constructors have bytecode
        const auto& constructors = classDef->getConstructors();
        for (const auto& constructor : constructors)
        {
            std::string constructorName = className + "::<init>/" +
                                        std::to_string(constructor->getParameterCount());
            if (!program.getFunction(constructorName))
            {
                throw errors::TypeException(
                    "Constructor for class '" + className + "' with " +
                    std::to_string(constructor->getParameterCount()) +
                    " parameters declared but not implemented. All constructors must have bytecode implementation.",
                    location
                );
            }
        }
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
}
