#include "ParameterValidator.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../runtime/utils/TypeConverter.hpp"

namespace vm::compiler::visitors
{
    ParameterValidator::ParameterValidator(CompilerContext& context)
        : ctx(context)
    {
    }

    const bytecode::BytecodeProgram::FunctionMetadata* ParameterValidator::resolveMethodMetadata(const std::string& qualifiedName)
    {
        // Try to find method metadata - first try with full qualified name
        const auto* methodMetadata = ctx.program.getFunction(qualifiedName);

        // If not found and qualified name contains generics, try stripping them
        // For example: "Container<Bool>::add" -> "Container::add"
        if (!methodMetadata && qualifiedName.find('<') != std::string::npos)
        {
            // Extract base class name without generic parameters
            size_t genericStart = qualifiedName.find('<');
            size_t methodSeparator = qualifiedName.find("::");

            if (genericStart < methodSeparator)
            {
                // Strip generics: "Container<Bool>::add" -> "Container::add"
                std::string baseClassName = qualifiedName.substr(0, genericStart);
                std::string methodPart = qualifiedName.substr(methodSeparator);
                std::string baseQualifiedName = baseClassName + methodPart;

                methodMetadata = ctx.program.getFunction(baseQualifiedName);
            }
        }

        return methodMetadata;
    }

    void ParameterValidator::validateParameterCount(const std::string& methodName,
                                                   const bytecode::BytecodeProgram::FunctionMetadata* methodMetadata,
                                                   size_t argumentCount, const ast::SourceLocation& location)
    {
        // For instance methods, parameterCount includes 'this', so subtract 1
        // For static methods, parameterCount is just the declared parameters
        size_t expectedParamCount = methodMetadata->parameterCount;
        if (!methodMetadata->isStatic && expectedParamCount > 0)
        {
            expectedParamCount -= 1; // Exclude 'this' from count
        }

        // Check parameter count
        if (expectedParamCount != argumentCount)
        {
            throw errors::EnvironmentException(
                "Method '" + methodName + "' expects " +
                std::to_string(expectedParamCount) +
                " parameter(s) but got " + std::to_string(argumentCount),
                location
            );
        }
    }

    bool ParameterValidator::validateGenericParameter(const std::string& methodName, const std::string& expectedType,
                                                      value::ValueType argType, const std::string& argTypeStr,
                                                      size_t paramIndex, const ast::SourceLocation& location)
    {
        // For unresolved generic type parameters (like K, V, T, E)
        if (expectedType.length() > 2 || !std::isupper(expectedType[0]))
        {
            return false; // Not a generic parameter
        }

        // If we can't infer the argument type (VOID), skip validation
        // This happens when accessing generic fields like current.data where data is of type T
        if (argType == value::ValueType::VOID)
        {
            return true; // Can't validate, rely on runtime
        }

        // Generic parameters represent OBJECT types (since primitives can't be generic type arguments)
        // So if argType is a primitive (and not VOID/NULL), this is an error
        if (argType != value::ValueType::OBJECT && argType != value::ValueType::NULL_TYPE)
        {
            throw errors::TypeException(
                "Method '" + methodName + "' parameter " + std::to_string(paramIndex + 1) +
                " expects an object type (generic parameter " + expectedType + ") but got primitive type " + argTypeStr +
                ". Cannot pass primitive types to generic parameters.",
                location
            );
        }
        // For OBJECT types, we can't validate the exact class without resolving the generic,
        // so we allow it and rely on runtime validation
        return true;
    }

    void ParameterValidator::validateSingleParameterType(const std::string& methodName, size_t paramIndex,
                                                         const std::string& expectedType, ast::ASTNode* argument,
                                                         const ast::SourceLocation& location)
    {
        // Resolve generic type parameters if present
        std::string resolvedExpectedType = ctx.resolveGenericType(expectedType);

        // Infer argument type early for validation checks
        value::ValueType argType = ctx.typeInference.inferExpressionType(argument);
        std::string argTypeStr = vm::runtime::utils::TypeConverter::valueTypeToString(argType);

        // Handle generic type parameters (T, K, V, E, etc.)
        if (validateGenericParameter(methodName, resolvedExpectedType, argType, argTypeStr, paramIndex, location))
        {
            return;
        }

        // Skip validation for generic array types (like T[], E[], Array<T>, etc.)
        if (resolvedExpectedType == "Array" || resolvedExpectedType.find("Array<") == 0)
        {
            // Check if argument is any array type (Int[], String[], etc.)
            std::string argClassName = ctx.typeInference.inferExpressionClassName(argument);
            if (argType == value::ValueType::OBJECT && argClassName.find("[]") != std::string::npos)
            {
                return; // Any array type is acceptable for generic array parameter
            }
        }

        // Check if expected type is a primitive
        bool isPrimitive = (resolvedExpectedType == "int" || resolvedExpectedType == "float" ||
            resolvedExpectedType == "string" || resolvedExpectedType == "bool" ||
            resolvedExpectedType == "void");

        if (isPrimitive)
        {
            // For primitive types, check exact match
            if (argType != value::ValueType::OBJECT && argTypeStr != resolvedExpectedType)
            {
                // Allow null for any type
                if (!dynamic_cast<ast::NullNode*>(argument))
                {
                    throw errors::TypeException(
                        "Method '" + methodName + "' parameter " + std::to_string(paramIndex + 1) +
                        " expects " + resolvedExpectedType + " but got " + argTypeStr,
                        location
                    );
                }
            }
        }
        else
        {
            // Expected type is an object/class
            if (argType != value::ValueType::OBJECT)
            {
                // null can be passed to object types
                if (!dynamic_cast<ast::NullNode*>(argument))
                {
                    throw errors::TypeException(
                        "Method '" + methodName + "' parameter " + std::to_string(paramIndex + 1) +
                        " expects " + resolvedExpectedType + " but got " + argTypeStr,
                        location
                    );
                }
            }
            else if (resolvedExpectedType != "object")
            {
                // For objects, check class name compatibility
                std::string argClassName = ctx.typeInference.inferExpressionClassName(argument);
                if (!argClassName.empty() && argClassName != resolvedExpectedType)
                {
                    // Strip generic parameters for comparison (List<int> -> List)
                    std::string expectedBase = resolvedExpectedType;
                    std::string argBase = argClassName;
                    size_t expectedAngle = resolvedExpectedType.find('<');
                    size_t argAngle = argClassName.find('<');
                    if (expectedAngle != std::string::npos)
                    {
                        expectedBase = resolvedExpectedType.substr(0, expectedAngle);
                    }
                    if (argAngle != std::string::npos)
                    {
                        argBase = argClassName.substr(0, argAngle);
                    }

                    // If base types match, it's compatible (generic type parameters will be validated at runtime)
                    if (expectedBase == argBase)
                    {
                        return; // Compatible generic types
                    }

                    // Check if argClassName is assignable to expectedType (inheritance)
                    if (!ctx.typeValidator.isClassCompatible(argClassName, resolvedExpectedType))
                    {
                        throw errors::TypeException(
                            "Method '" + methodName + "' parameter " + std::to_string(paramIndex + 1) +
                            " expects " + resolvedExpectedType + " but got " + argClassName,
                            location
                        );
                    }
                }
            }
        }
    }

    void ParameterValidator::validateMethodParameters(
        const std::string& methodName,
        const std::string& qualifiedName,
        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
        const ast::SourceLocation& location)
    {
        // Resolve method metadata
        const auto* methodMetadata = resolveMethodMetadata(qualifiedName);

        // Skip validation if method not found (could be native or external library)
        if (!methodMetadata || methodMetadata->isNative)
        {
            return;
        }

        // Validate parameter count
        validateParameterCount(methodName, methodMetadata, arguments.size(), location);

        // For instance methods, parameterTypes[0] is 'this', so we skip it
        size_t parameterTypeOffset = (!methodMetadata->isStatic && !methodMetadata->parameterTypes.empty()) ? 1 : 0;

        // Validate parameter types
        if (methodMetadata->parameterTypes.size() <= parameterTypeOffset)
        {
            return; // No actual parameters to validate (only 'this' or empty)
        }

        for (size_t i = 0; i < arguments.size(); ++i)
        {
            // Make sure we don't go out of bounds
            if (i + parameterTypeOffset >= methodMetadata->parameterTypes.size())
            {
                break;
            }

            std::string expectedType = methodMetadata->parameterTypes[i + parameterTypeOffset];
            validateSingleParameterType(methodName, i, expectedType, arguments[i].get(), location);
        }
    }

    void ParameterValidator::validateConstructorParameters(const std::vector<std::unique_ptr<ast::ASTNode>>& arguments,
                                                          const runtimeTypes::klass::ConstructorDefinition* constructor,
                                                          const ast::SourceLocation& location)
    {
        if (!constructor)
        {
            return; // No validation if constructor not found
        }

        const auto& params = constructor->getParametersWithTypes();
        for (size_t i = 0; i < arguments.size(); ++i)
        {
            const auto& paramType = params[i].second;
            value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

            // Skip validation if we can't infer the type
            if (argType == value::ValueType::VOID)
            {
                continue;
            }

            // For object types, check class names
            if (paramType.basicType == value::ValueType::OBJECT && paramType.className.has_value())
            {
                std::string expectedClass = paramType.className.value();

                // Skip generic type parameters (single uppercase letters)
                if (expectedClass.length() <= 2 && std::isupper(expectedClass[0]))
                {
                    continue;
                }

                if (argType != value::ValueType::OBJECT)
                {
                    // Allow null
                    if (!dynamic_cast<ast::NullNode*>(arguments[i].get()))
                    {
                        std::string argTypeStr = vm::runtime::utils::TypeConverter::valueTypeToString(argType);
                        throw errors::TypeException(
                            "Constructor parameter " + std::to_string(i + 1) +
                            " expects " + expectedClass + " but got " + argTypeStr,
                            location
                        );
                    }
                }
                else
                {
                    // Check class name match
                    std::string argClassName = ctx.typeInference.inferExpressionClassName(arguments[i].get());
                    if (!argClassName.empty() && argClassName != expectedClass && expectedClass != "object")
                    {
                        if (!dynamic_cast<ast::NullNode*>(arguments[i].get()))
                        {
                            throw errors::TypeException(
                                "Constructor parameter " + std::to_string(i + 1) +
                                " expects " + expectedClass + " but got " + argClassName,
                                location
                            );
                        }
                    }
                }
            }
            // For primitive types
            else
            {
                std::string expectedTypeStr = vm::runtime::utils::TypeConverter::valueTypeToString(paramType.basicType);
                std::string argTypeStr = vm::runtime::utils::TypeConverter::valueTypeToString(argType);

                if (paramType.basicType != argType)
                {
                    // Allow int to float conversion
                    if (!(paramType.basicType == value::ValueType::FLOAT && argType == value::ValueType::INT))
                    {
                        throw errors::TypeException(
                            "Constructor parameter " + std::to_string(i + 1) +
                            " expects " + expectedTypeStr + " but got " + argTypeStr,
                            location
                        );
                    }
                }
            }
        }
    }
}
