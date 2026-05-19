#include "CompileTimeValidator.hpp"
#include <cstddef>
#include "../../MethodSignature.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"

namespace vm::compiler::validation
{
    void CompileTimeValidator::validateTypeIsNotRawGeneric(const std::string& typeName,
                                                          const ast::SourceLocation& location)
    {
        // Skip validation for:
        // 1. Generic type parameters (T, K, V, E, etc.) - single uppercase letters
        // 2. Primitive types (int, float, string, bool, void)
        // 3. Type names that already contain generic arguments (e.g., "Wrapper<Int>")

        if (typeName.length() == 1 && std::isupper(typeName[0]))
        {
            return;
        }

        if (typeName.find('<') != std::string::npos)
        {
            return;
        }

        if (typeName == "int" || typeName == "float" || typeName == "string" ||
            typeName == "bool" || typeName == "void" || typeName == "object")
        {
            return;
        }

        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry)
        {
            return;
        }

        auto classDef = classRegistry->findClass(typeName);
        if (classDef && !classDef->getGenericParameters().empty())
        {
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

        auto interfaceDef = environment->findInterface(typeName);
        if (interfaceDef && !interfaceDef->getGenericParameters().empty())
        {
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
        auto classRegistry = environment->getClassRegistry();
        auto classDef = classRegistry->findClass(className);
        if (!classDef)
        {
            return; // Class doesn't exist - caught elsewhere
        }

        const auto& methods = classDef->getInstanceMethods();
        for (const auto& [methodName, overloads] : methods)
        {
            for (const auto& methodDef : overloads)
            {
                // MethodSignature handles arrays, generics, and 'this'-vs-static disambiguation.
                auto signature = vm::MethodSignature::fromMethodDefinition(methodDef.get());
                std::string qualifiedName = signature.toMangledName(className, false);

                if (!program.getFunction(qualifiedName))
                {
                    throw errors::TypeException(
                        "Instance method '" + qualifiedName +
                        "' declared but not implemented. All instance methods must have bytecode implementation.",
                        location
                    );
                }
            }
        }

        const auto& staticMethods = classDef->getStaticMethods();
        for (const auto& [methodName, overloads] : staticMethods)
        {
            for (const auto& methodDef : overloads)
            {
                auto signature = vm::MethodSignature::fromMethodDefinition(methodDef.get());
                std::string qualifiedName = signature.toMangledName(className, true);

                if (!program.getFunction(qualifiedName))
                {
                    throw errors::TypeException(
                        "Static method '" + qualifiedName +
                        "' declared but not implemented. All static methods must have bytecode implementation.",
                        location
                    );
                }
            }
        }

        const auto& constructors = classDef->getConstructors();
        for (const auto& constructor : constructors)
        {
            std::string typeSignature = constructor->getTypeSignature();

            std::string constructorName;
            if (typeSignature.empty()) {
                constructorName = className + "::<init>";
            } else {
                constructorName = className + "::<init>/" + typeSignature;
            }

            if (!program.getFunction(constructorName))
            {
                throw errors::TypeException(
                    "Constructor for class '" + className + "' with signature (" + typeSignature + ") " +
                    "declared but not implemented. All constructors must have bytecode implementation.",
                    location
                );
            }
        }
    }
}
