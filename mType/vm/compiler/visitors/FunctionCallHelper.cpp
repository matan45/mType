#include "FunctionCallHelper.hpp"
#include "../validation/CompileTimeValidator.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../bytecode/OpCode.hpp"
#include <optional>
#include <iostream>

namespace vm::compiler::visitors
{
    FunctionCallHelper::FunctionCallHelper(CompilerContext& context)
        : ctx(context)
    {
    }

    std::string FunctionCallHelper::inferTypeFromArgument(ast::ASTNode* argument)
    {
        // Infer the type of an argument for generic type inference
        value::ValueType argType = ctx.typeInference.inferExpressionType(argument);

        if (argType == value::ValueType::VOID)
        {
            return ""; // Cannot infer - unknown type
        }

        // For primitive types, return the type name
        if (argType != value::ValueType::OBJECT)
        {
            return ::types::TypeConversionUtils::getTypeDisplayName(argType);
        }

        // For object types, return the class name
        std::string className = ctx.typeInference.inferExpressionClassName(argument);
        return className;
    }

    bool FunctionCallHelper::setupGenericTypeBindings(ast::FunctionCallNode* node, const std::string& functionName)
    {
        const auto* funcMetadata = ctx.program.getFunction(functionName);
        if (!funcMetadata)
        {
            // Function not yet registered - this can happen for forward references
            // or if the function is defined later in the file
            // In this case, we can't perform type inference without the signature
            return false;
        }

        // If not a generic function, nothing to do
        if (funcMetadata->genericTypeParameters.empty())
        {
            // But if user provided type arguments, that's an error
            if (node->hasGenericTypeArguments())
            {
                throw errors::TypeException(
                    "Function '" + functionName + "' is not generic but generic type arguments were provided",
                    node->getLocation());
            }
            return false;
        }

        std::unordered_map<std::string, std::string> typeBindings;

        if (node->hasGenericTypeArguments())
        {
            // Explicit type arguments provided - use them
            const auto& genericTypeArgs = node->getGenericTypeArguments();
            const auto& genericTypeParams = funcMetadata->genericTypeParameters;

            // Validate that the number of type arguments matches the number of type parameters
            if (genericTypeArgs.size() != genericTypeParams.size())
            {
                throw errors::TypeException(
                    "Function '" + functionName + "' expects " +
                    std::to_string(genericTypeParams.size()) + " type argument(s) but got " +
                    std::to_string(genericTypeArgs.size()),
                    node->getLocation());
            }

            // Build type bindings map
            for (size_t i = 0; i < genericTypeParams.size(); ++i)
            {
                typeBindings[genericTypeParams[i]] = genericTypeArgs[i];
            }
        }
        else
        {
            // No explicit type arguments - infer from call arguments
            const auto& arguments = node->getArguments();
            const auto& genericTypeParams = funcMetadata->genericTypeParameters;

            // If the function has no parameters, we can't infer type arguments
            if (funcMetadata->parameterTypes.empty() || arguments.empty())
            {
                // Can't infer without parameters/arguments - require explicit type arguments
                throw errors::TypeException(
                    "Cannot infer type arguments for generic function '" + functionName +
                    "'. Provide explicit type arguments like '" + functionName + "<Type>(...)'",
                    node->getLocation());
            }

            // For each generic type parameter, collect all inferred types and unify them
            for (const auto& typeParam : genericTypeParams)
            {
                std::optional<std::string> unifiedType;
                std::vector<std::pair<size_t, std::string>> inferredTypes; // (paramIndex, type)

                // Find all parameter positions that use this type parameter
                for (size_t i = 0; i < funcMetadata->parameterTypes.size(); ++i)
                {
                    const std::string& paramType = funcMetadata->parameterTypes[i];

                    // Check if this parameter type is exactly the type parameter (e.g., "T")
                    if (paramType == typeParam)
                    {
                        if (i < arguments.size())
                        {
                            std::string argType = inferTypeFromArgument(arguments[i].get());

                            if (argType.empty())
                            {
                                // Cannot infer type from this argument
                                continue;
                            }

                            inferredTypes.push_back({i, argType});

                            if (!unifiedType.has_value())
                            {
                                unifiedType = argType;
                            }
                            else if (unifiedType.value() != argType)
                            {
                                // UNIFICATION FAILURE - conflicting types
                                throw errors::TypeException(
                                    "Cannot infer type parameter '" + typeParam + "' for function '" +
                                    functionName + "': parameter " + std::to_string(inferredTypes[0].first + 1) +
                                    " has type " + inferredTypes[0].second + " but parameter " +
                                    std::to_string(i + 1) + " has type " + argType,
                                    node->getLocation());
                            }
                        }
                    }
                }

                if (!unifiedType.has_value())
                {
                    // No arguments to infer from - this means the type parameter isn't used in any parameter
                    // It might be used only in the return type, which we don't currently support for inference
                    // For now, require explicit type arguments
                    throw errors::TypeException(
                        "Cannot infer type parameter '" + typeParam + "' for function '" +
                        functionName + "': type parameter not used in function parameters. " +
                        "Provide explicit type arguments like '" + functionName + "<Type>(...)'",
                        node->getLocation());
                }

                typeBindings[typeParam] = unifiedType.value();
            }
        }

        // Push bindings onto stack
        ctx.pushGenericTypeBindings(typeBindings);
        return true;
    }

    void FunctionCallHelper::validateFunctionParameters(ast::FunctionCallNode* node, const std::string& functionName,
                                                        const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        // Skip validation for method calls (contain "::")
        if (functionName.find("::") != std::string::npos)
        {
            return;
        }

        // Check if function is registered
        const auto* funcMetadata = ctx.program.getFunction(functionName);
        if (!funcMetadata)
        {
            return;
        }

        // Skip all validation for native functions (they handle their own parameter checking at runtime)
        if (funcMetadata->isNative)
        {
            return;
        }

        if (funcMetadata->parameterCount != arguments.size())
        {
            throw errors::EnvironmentException(
                "Function '" + functionName + "' expects " +
                std::to_string(funcMetadata->parameterCount) +
                " parameter(s) but got " + std::to_string(arguments.size()),
                node->getLocation()
            );
        }

        // Validate parameter types
        if (funcMetadata->parameterTypes.empty())
        {
            return;
        }

        for (size_t i = 0; i < arguments.size(); ++i)
        {
            std::string expectedType = funcMetadata->parameterTypes[i];

            // Resolve generic type parameters if present
            expectedType = ctx.resolveGenericType(expectedType);

            value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

            // Skip validation if type inference failed (returned void/unknown)
            if (argType == value::ValueType::VOID)
            {
                continue; // Can't validate - inference failed or expression is void
            }

            // Convert argType to string for comparison
            std::string argTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(argType);

            // For object types, need to check class names
            if (expectedType != "int" && expectedType != "float" &&
                expectedType != "string" && expectedType != "bool" &&
                expectedType != "void")
            {
                // Expected type is an object/class
                if (argType != value::ValueType::OBJECT)
                {
                    // null can be passed to object types
                    if (!dynamic_cast<ast::NullNode*>(arguments[i].get()))
                    {
                        // PHASE 4: Allow primitive literals for Box types (auto-boxing)
                        bool canAutoBox = false;
                        if ((expectedType == "Int" && argType == value::ValueType::INT) ||
                            (expectedType == "Float" && argType == value::ValueType::FLOAT) ||
                            (expectedType == "Bool" && argType == value::ValueType::BOOL) ||
                            (expectedType == "String" && argType == value::ValueType::STRING))
                        {
                            canAutoBox = true;
                        }

                        if (!canAutoBox)
                        {
                            throw errors::TypeException(
                                "Function '" + functionName + "' parameter " + std::to_string(i + 1) +
                                " expects " + expectedType + " but got " + argTypeStr,
                                node->getLocation()
                            );
                        }
                    }
                }
                // For objects, check class name compatibility
                else if (expectedType != "object")
                {
                    std::string argClassName = ctx.typeInference.inferExpressionClassName(arguments[i].get());

                    // Normalize array types: convert "int[]" to "Array<int>", "int[][]" to "Array<Array<int>>", etc.
                    auto normalizeArrayType = [](const std::string& type) -> std::string
                    {
                        std::string normalized = type;
                        size_t arrayDepth = 0;

                        // Count array brackets from the end
                        while (normalized.length() >= 2 && normalized.substr(normalized.length() - 2) == "[]")
                        {
                            arrayDepth++;
                            normalized = normalized.substr(0, normalized.length() - 2);
                        }

                        // Wrap in Array<> for each dimension
                        for (size_t i = 0; i < arrayDepth; ++i)
                        {
                            normalized = "Array<" + normalized + ">";
                        }

                        return normalized;
                    };

                    std::string normalizedArgClassName = normalizeArrayType(argClassName);
                    std::string normalizedExpectedType = normalizeArrayType(expectedType);

                    if (!argClassName.empty() && normalizedArgClassName != normalizedExpectedType)
                    {
                        // Check if both are generic types with the same base
                        bool isGenericMatch = false;
                        size_t expectedAngle = normalizedExpectedType.find('<');
                        size_t argAngle = normalizedArgClassName.find('<');

                        if (expectedAngle != std::string::npos && argAngle != std::string::npos)
                        {
                            // Both are generic types - check if base types match
                            std::string expectedBase = normalizedExpectedType.substr(0, expectedAngle);
                            std::string argBase = normalizedArgClassName.substr(0, argAngle);
                            if (expectedBase == argBase)
                            {
                                // Same generic base type (e.g., both are List, both are Array)
                                // Type argument compatibility will be validated at runtime
                                isGenericMatch = true;
                            }
                        }

                        if (!isGenericMatch)
                        {
                            // Check if argClassName is assignable to expectedType (inheritance/polymorphism)
                            if (!ctx.typeValidator.isClassCompatible(argClassName, expectedType))
                            {
                                // null can be passed to any object type
                                if (!dynamic_cast<ast::NullNode*>(arguments[i].get()))
                                {
                                    throw errors::TypeException(
                                        "Function '" + functionName + "' parameter " + std::to_string(i + 1) +
                                        " expects " + expectedType + " but got " + argClassName,
                                        node->getLocation()
                                    );
                                }
                            }
                        }
                    }
                }
            }
            // For primitive types, check exact match (with int->float exception)
            else if (expectedType != argTypeStr)
            {
                // Allow int to float conversion
                if (!(expectedType == "float" && argTypeStr == "int"))
                {
                    throw errors::TypeException(
                        "Function '" + functionName + "' parameter " + std::to_string(i + 1) +
                        " expects " + expectedType + " but got " + argTypeStr,
                        node->getLocation()
                    );
                }
            }
        }
    }

    void FunctionCallHelper::emitStaticMethodCall(ast::FunctionCallNode* node, const std::string& functionName,
                                                  const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        // Create mutable copy for potential modification
        std::string actualFunctionName = functionName;

        // Replace "this::" with actual class name if inside a class
        if (actualFunctionName.substr(0, 6) == "this::" && ctx.currentClassNode)
        {
            std::string methodName = actualFunctionName.substr(6); // Remove "this::"
            actualFunctionName = ctx.currentClassNode->getClassName() + "::" + methodName;
        }

        // Validate static method exists at compile time
        if (ctx.compileTimeValidator)
        {
            // Extract className and methodName from qualified name
            size_t colonPos = actualFunctionName.find("::");
            if (colonPos != std::string::npos)
            {
                std::string className = actualFunctionName.substr(0, colonPos);
                std::string methodName = actualFunctionName.substr(colonPos + 2);
                ctx.compileTimeValidator->validateStaticMethodExists(className, methodName,
                                                                     arguments.size(), node->getLocation());
            }
        }

        // Compile all arguments (left to right) with auto-boxing support
        // Note: For static method calls, we don't have easy access to parameter types
        // Auto-boxing will be handled at the parser level or via explicit conversion
        // For now, compile arguments normally
        for (const auto& arg : arguments)
        {
            arg->accept(ctx.visitor);
        }

        // Note: We don't append "$static" here because the CALL_STATIC opcode handler
        // will append it when looking up the bytecode
        size_t nameIndex = ctx.program.getConstantPool().addString(actualFunctionName);
        // Static method call - use CALL_STATIC with source location
        ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_STATIC,
                                     static_cast<uint32_t>(nameIndex),
                                     static_cast<uint32_t>(arguments.size()), node);
    }

    void FunctionCallHelper::emitMethodCallInClassContext(ast::FunctionCallNode* node, const std::string& functionName,
                                                          const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        // Unqualified call inside a method (instance or static) - could be either:
        // 1. Method call on 'this' (for instance methods)
        // 2. Static method call (for static methods)
        // 3. Regular function call (global function)
        bool isMethodCall = false;
        bool isStaticMethodCall = false;
        const auto& methods = ctx.currentClassNode->getMethods();

        for (const auto& method : methods)
        {
            if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get()))
            {
                if (methodNode->getName() == functionName &&
                    methodNode->getParameterCount() == arguments.size())
                {
                    // If we're in a static method, only match static methods
                    // If we're in an instance method, prefer instance methods
                    if (ctx.inStaticMethod)
                    {
                        if (methodNode->getIsStatic())
                        {
                            isMethodCall = true;
                            isStaticMethodCall = true;
                            break;
                        }
                    }
                    else if (ctx.inInstanceMethod)
                    {
                        if (!methodNode->getIsStatic())
                        {
                            isMethodCall = true;
                            isStaticMethodCall = false;
                            break;
                        }
                    }
                }
            }
        }

        // If we didn't find a matching method in the preferred namespace,
        // check the other namespace (only in instance context, static can also call instance)
        if (!isMethodCall && ctx.inInstanceMethod)
        {
            for (const auto& method : methods)
            {
                if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get()))
                {
                    if (methodNode->getName() == functionName &&
                        methodNode->getParameterCount() == arguments.size() &&
                        methodNode->getIsStatic())
                    {
                        isMethodCall = true;
                        isStaticMethodCall = true;
                        break;
                    }
                }
            }
        }

        if (isMethodCall)
        {
            if (isStaticMethodCall)
            {
                // Static method call - use CALL_STATIC with fully qualified name
                std::string qualifiedName = ctx.currentClassNode->getClassName() + "::" + functionName;

                // Validate static method exists at compile time
                if (ctx.compileTimeValidator)
                {
                    ctx.compileTimeValidator->validateStaticMethodExists(
                        ctx.currentClassNode->getClassName(), functionName,
                        arguments.size(), node->getLocation());
                }

                // Compile all arguments
                for (const auto& arg : arguments)
                {
                    arg->accept(ctx.visitor);
                }

                size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_STATIC,
                                             static_cast<uint32_t>(nameIndex),
                                             static_cast<uint32_t>(arguments.size()), node);
            }
            else
            {
                // Instance method call - push 'this' onto stack BEFORE arguments

                // Validate instance method exists at compile time
                if (ctx.compileTimeValidator)
                {
                    ctx.compileTimeValidator->validateInstanceMethodExists(
                        ctx.currentClassNode->getClassName(), functionName,
                        arguments.size(), node->getLocation());
                }

                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(0), node);

                // Now compile arguments
                for (const auto& arg : arguments)
                {
                    arg->accept(ctx.visitor);
                }

                size_t nameIndex = ctx.program.getConstantPool().addString(functionName);
                // Call method on 'this' with source location
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint32_t>(nameIndex),
                                             static_cast<uint32_t>(arguments.size()), node);
            }
        }
        else
        {
            // Regular function call
            emitRegularFunctionCall(node, functionName, arguments);
        }
    }

    void FunctionCallHelper::emitRegularFunctionCall(ast::FunctionCallNode* node, const std::string& functionName,
                                                     const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        // Validate function exists at compile time (all functions are pre-registered)
        if (ctx.compileTimeValidator)
        {
            ctx.compileTimeValidator->validateFunctionExists(functionName, node->getLocation());
        }

        // Get function metadata for parameter type information (for auto-boxing)
        const auto* funcMetadata = ctx.program.getFunction(functionName);

        // Compile all arguments (left to right) with auto-boxing/unboxing support
        for (size_t i = 0; i < arguments.size(); ++i)
        {
            // PHASE 4: Apply auto-boxing/unboxing if function metadata available
            if (funcMetadata && i < funcMetadata->parameterTypes.size())
            {
                std::string expectedType = ctx.resolveGenericType(funcMetadata->parameterTypes[i]);
                value::ValueType expectedValueType = ::types::TypeConversionUtils::stringToValueType(expectedType);

                compileArgumentWithAutoBoxing(arguments[i].get(), expectedType, expectedValueType);
            }
            else
            {
                // No metadata - but still try auto-unboxing for common native functions
                // For functions like print(), we want to auto-unbox String objects
                value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

                std::cerr << "[DEBUG] emitRegularFunctionCall: function=" << functionName
                          << ", arg=" << i << ", argType=" << static_cast<int>(argType) << std::endl;

                if (argType == value::ValueType::OBJECT)
                {
                    std::string argClassName = ctx.typeInference.inferExpressionClassName(arguments[i].get());

                    std::cerr << "[DEBUG] argClassName=" << argClassName << std::endl;

                    // Auto-unbox Box types for native functions
                    if (argClassName == "Int" || argClassName == "Float" ||
                        argClassName == "Bool" || argClassName == "String")
                    {
                        std::cerr << "[DEBUG] AUTO-UNBOXING " << argClassName << " for native function" << std::endl;

                        // Compile argument (Box object)
                        arguments[i]->accept(ctx.visitor);

                        // Call getValue() to extract primitive
                        size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                        ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                                     static_cast<uint32_t>(methodNameIndex),
                                                     0u,  // 0 arguments
                                                     arguments[i].get());
                    }
                    else
                    {
                        std::cerr << "[DEBUG] Not a Box type, compile normally" << std::endl;
                        // Not a Box type, compile normally
                        arguments[i]->accept(ctx.visitor);
                    }
                }
                else
                {
                    std::cerr << "[DEBUG] Not OBJECT type, compile normally" << std::endl;
                    // Compile normally
                    arguments[i]->accept(ctx.visitor);
                }
            }
        }

        size_t nameIndex = ctx.program.getConstantPool().addString(functionName);
        // Regular function call - use CALL with source location
        ctx.emitter.emitWithLocation(bytecode::OpCode::CALL,
                                     static_cast<uint32_t>(nameIndex),
                                     static_cast<uint32_t>(arguments.size()), node);
    }

    value::Value FunctionCallHelper::compileFunctionCall(ast::FunctionCallNode* node)
    {
        // Get function name and arguments
        std::string functionName = node->getFunctionName();
        const auto& arguments = node->getArguments();

        // Setup generic type bindings if needed
        bool hasGenericBindings = setupGenericTypeBindings(node, functionName);

        // Validate function parameters (count and types)
        validateFunctionParameters(node, functionName, arguments);

        // Emit appropriate bytecode based on call type
        if (functionName.find("::") != std::string::npos)
        {
            // Static method call (ClassName::methodName)
            emitStaticMethodCall(node, functionName, arguments);
        }
        else if ((ctx.inInstanceMethod || ctx.inStaticMethod) && ctx.currentClassNode)
        {
            // Unqualified call inside a method - could be instance method, static method, or function
            emitMethodCallInClassContext(node, functionName, arguments);
        }
        else
        {
            // Regular function call
            emitRegularFunctionCall(node, functionName, arguments);
        }

        // Pop generic type bindings if we pushed them
        if (hasGenericBindings)
        {
            ctx.popGenericTypeBindings();
        }

        return std::monostate{};
    }

    // Phase 4: Auto-boxing/unboxing helper for function arguments
    void FunctionCallHelper::compileArgumentWithAutoBoxing(ast::ASTNode* argument,
                                                          const std::string& expectedTypeName,
                                                          value::ValueType expectedType)
    {
        using namespace ast::nodes::expressions;

        // Get the actual type of the argument
        value::ValueType actualType = ctx.typeInference.inferExpressionType(argument);

        // PHASE 4: Auto-unboxing - if expected is primitive but argument is Box object
        if (expectedType != value::ValueType::OBJECT && actualType == value::ValueType::OBJECT)
        {
            // Check if argument is a Box type that should be unboxed
            std::string actualClassName = ctx.typeInference.inferExpressionClassName(argument);

            bool canAutoUnbox = false;
            if ((expectedType == value::ValueType::INT && actualClassName == "Int") ||
                (expectedType == value::ValueType::FLOAT && actualClassName == "Float") ||
                (expectedType == value::ValueType::BOOL && actualClassName == "Bool") ||
                (expectedType == value::ValueType::STRING && actualClassName == "String"))
            {
                canAutoUnbox = true;
            }

            if (canAutoUnbox)
            {
                // Auto-unbox: compile argument (Box object) then call getValue()
                argument->accept(ctx.visitor);

                // Call getValue() method to extract primitive value
                size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint32_t>(methodNameIndex),
                                             0u,  // 0 arguments
                                             argument);
                return;
            }

            // Can't auto-unbox, compile normally
            argument->accept(ctx.visitor);
            return;
        }

        // PHASE 4: Auto-boxing - if expected is Box object but argument is primitive
        if (expectedType != value::ValueType::OBJECT)
        {
            // Not an object type, compile normally
            argument->accept(ctx.visitor);
            return;
        }

        // Check if expected type is a primitive Box type
        bool isBoxType = (expectedTypeName == "Int" ||
                          expectedTypeName == "Float" ||
                          expectedTypeName == "Bool" ||
                          expectedTypeName == "String");

        if (!isBoxType)
        {
            // Not a Box type, compile normally
            argument->accept(ctx.visitor);
            return;
        }

        // Check if argument is a primitive literal that needs boxing
        bool needsBoxing = false;

        if (expectedTypeName == "Int" && dynamic_cast<IntegerNode*>(argument))
        {
            needsBoxing = true;
        }
        else if (expectedTypeName == "Float" && dynamic_cast<FloatNode*>(argument))
        {
            needsBoxing = true;
        }
        else if (expectedTypeName == "Bool" && dynamic_cast<BoolNode*>(argument))
        {
            needsBoxing = true;
        }
        else if (expectedTypeName == "String" && dynamic_cast<StringNode*>(argument))
        {
            needsBoxing = true;
        }

        if (!needsBoxing)
        {
            // Argument is not a primitive literal, compile normally
            argument->accept(ctx.visitor);
            return;
        }

        // PHASE 4 AUTO-BOXING: Emit bytecode for boxing
        // Equivalent to: new ExpectedType(literalValue)

        // 1. Compile the literal value (pushes it onto stack)
        argument->accept(ctx.visitor);

        // 2. Emit NEW_OBJECT for the Box class
        size_t classNameIndex = ctx.program.getConstantPool().addString(expectedTypeName);
        ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                     static_cast<uint32_t>(classNameIndex),
                                     1u,  // 1 constructor argument
                                     argument);
    }
}
