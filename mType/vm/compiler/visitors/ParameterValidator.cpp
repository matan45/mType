#include "ParameterValidator.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../types/GenericTypeResolver.hpp"

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

    std::string ParameterValidator::normalizeTypeString(const std::string& typeStr)
    {
        // Normalize type strings by removing spaces around commas and angle brackets
        // This ensures "HashMap<Int, String>" matches "HashMap<Int,String>"
        std::string normalized;
        normalized.reserve(typeStr.size());

        for (size_t i = 0; i < typeStr.length(); ++i)
        {
            char c = typeStr[i];

            // Skip spaces after commas and angle brackets
            if (c == ' ')
            {
                if (i > 0 && (typeStr[i - 1] == ',' || typeStr[i - 1] == '<'))
                {
                    continue; // Skip space after comma or <
                }
                if (i + 1 < typeStr.length() && typeStr[i + 1] == '>')
                {
                    continue; // Skip space before >
                }
            }

            normalized += c;
        }

        return normalized;
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
        std::string argTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(argType);

        // Handle generic type parameters (T, K, V, E, etc.)
        if (validateGenericParameter(methodName, resolvedExpectedType, argType, argTypeStr, paramIndex, location))
        {
            return;
        }

        // Skip validation for array types (like T[], E[], Array<T>, int[], etc.)
        if (resolvedExpectedType == "Array" || resolvedExpectedType.find("Array<") == 0 ||
            resolvedExpectedType.find("[]") != std::string::npos)
        {
            // Check if argument is any array type
            if (argType == value::ValueType::ARRAY)
            {
                return; // Any array type is acceptable for array parameter
            }
            // Also check for object arrays (like Int[], String[] which are stored as objects)
            std::string argClassName = ctx.typeInference.inferExpressionClassName(argument);
            if (argType == value::ValueType::OBJECT && argClassName.find("[]") != std::string::npos)
            {
                return; // Object array types are also acceptable
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
                    // Check if auto-boxing can handle this (primitive -> boxed type)
                    bool canAutoBox = false;
                    if ((resolvedExpectedType == "Int" && argType == value::ValueType::INT) ||
                        (resolvedExpectedType == "Float" && argType == value::ValueType::FLOAT) ||
                        (resolvedExpectedType == "Bool" && argType == value::ValueType::BOOL) ||
                        (resolvedExpectedType == "String" && argType == value::ValueType::STRING))
                    {
                        canAutoBox = true;
                    }

                    if (!canAutoBox)
                    {
                        throw errors::TypeException(
                            "Method '" + methodName + "' parameter " + std::to_string(paramIndex + 1) +
                            " expects " + resolvedExpectedType + " but got " + argTypeStr,
                            location
                        );
                    }
                }
            }
            else if (resolvedExpectedType != "object")
            {
                // For objects, check class name compatibility
                std::string argClassName = ctx.typeInference.inferExpressionClassName(argument);
                if (!argClassName.empty())
                {
                    // Normalize both type strings to handle whitespace differences
                    std::string normalizedExpected = normalizeTypeString(resolvedExpectedType);
                    std::string normalizedArg = normalizeTypeString(argClassName);

                    if (normalizedArg != normalizedExpected)
                    {
                        // Strip generic parameters for comparison (List<int> -> List)
                        std::string expectedBase = normalizedExpected;
                        std::string argBase = normalizedArg;
                        size_t expectedAngle = normalizedExpected.find('<');
                        size_t argAngle = normalizedArg.find('<');
                        if (expectedAngle != std::string::npos)
                        {
                            expectedBase = normalizedExpected.substr(0, expectedAngle);
                        }
                        if (argAngle != std::string::npos)
                        {
                            argBase = normalizedArg.substr(0, argAngle);
                        }

                        // If base types match, it's compatible (generic type parameters will be validated at runtime)
                        if (expectedBase == argBase)
                        {
                            return; // Compatible generic types
                        }

                        // PHASE 4: Allow generic type parameters to pass validation
                        // If argClassName is a single uppercase letter (T, E, K, V, R, etc.), it's likely a generic type parameter
                        // These will be bound to concrete types at call time, so allow them through
                        bool isGenericTypeParameter = (argClassName.length() <= 2 &&
                                                       !argClassName.empty() &&
                                                       std::isupper(argClassName[0]));

                        // Check if argClassName is assignable to expectedType (inheritance)
                        if (!isGenericTypeParameter && !ctx.typeValidator.isClassCompatible(argClassName, resolvedExpectedType))
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
                                                          const ast::SourceLocation& location,
                                                          const std::unordered_map<std::string, std::string>& genericTypeBindings)
    {
        if (!constructor)
        {
            return; // No validation if constructor not found
        }

        // Create resolver for generic type substitution
        types::GenericTypeResolver resolver;

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

            // For object types and array types, check class names
            if ((paramType.basicType == value::ValueType::OBJECT || paramType.basicType == value::ValueType::ARRAY)
                && paramType.className.has_value())
            {
                std::string expectedClass = paramType.className.value();

                // Resolve generic type parameters using the bindings
                // This transforms "TypeToken<T>" -> "TypeToken<Int>" when T=Int
                if (!genericTypeBindings.empty())
                {
                    expectedClass = resolver.resolveGenericType(expectedClass, genericTypeBindings);
                }

                // Skip generic type parameters (single uppercase letters) that weren't resolved
                if (expectedClass.length() <= 2 && std::isupper(expectedClass[0]))
                {
                    continue;
                }

                if (argType != value::ValueType::OBJECT && argType != value::ValueType::ARRAY)
                {
                    // Allow null
                    if (!dynamic_cast<ast::NullNode*>(arguments[i].get()))
                    {
                        std::string argTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(argType);
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
                    if (!argClassName.empty() && expectedClass != "object")
                    {
                        // Normalize both type strings to handle whitespace differences
                        std::string normalizedExpected = normalizeTypeString(expectedClass);
                        std::string normalizedArg = normalizeTypeString(argClassName);

                        if (normalizedArg != normalizedExpected)
                        {
                            // Check if argClassName is assignable to expectedClass (inheritance/polymorphism)
                            if (!ctx.typeValidator.isClassCompatible(argClassName, expectedClass))
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
                }
            }
            // For primitive types
            else
            {
                std::string expectedTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(paramType.basicType);
                std::string argTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(argType);

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
