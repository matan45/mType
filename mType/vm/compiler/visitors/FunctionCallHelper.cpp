#include "FunctionCallHelper.hpp"
#include "../validation/CompileTimeValidator.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../runtime/utils/TypeConverter.hpp"
#include "../../bytecode/OpCode.hpp"

namespace vm::compiler::visitors
{
    FunctionCallHelper::FunctionCallHelper(CompilerContext& context)
        : ctx(context)
    {
    }

    bool FunctionCallHelper::setupGenericTypeBindings(ast::FunctionCallNode* node, const std::string& functionName)
    {
        if (!node->hasGenericTypeArguments()) {
            return false;
        }

        const auto* funcMetadata = ctx.program.getFunction(functionName);
        if (!funcMetadata) {
            return false;
        }

        // Validate that the function is actually generic
        if (funcMetadata->genericTypeParameters.empty()) {
            throw errors::TypeException(
                "Function '" + functionName + "' is not generic but generic type arguments were provided",
                node->getLocation());
        }

        const auto& genericTypeArgs = node->getGenericTypeArguments();
        const auto& genericTypeParams = funcMetadata->genericTypeParameters;

        // Validate that the number of type arguments matches the number of type parameters
        if (genericTypeArgs.size() != genericTypeParams.size()) {
            throw errors::TypeException(
                "Function '" + functionName + "' expects " +
                std::to_string(genericTypeParams.size()) + " type argument(s) but got " +
                std::to_string(genericTypeArgs.size()),
                node->getLocation());
        }

        // Build type bindings map
        std::unordered_map<std::string, std::string> typeBindings;
        for (size_t i = 0; i < genericTypeParams.size(); ++i) {
            typeBindings[genericTypeParams[i]] = genericTypeArgs[i];
        }

        // Push bindings onto stack
        ctx.pushGenericTypeBindings(typeBindings);
        return true;
    }

    void FunctionCallHelper::validateFunctionParameters(ast::FunctionCallNode* node, const std::string& functionName,
                                                      const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        // Skip validation for method calls (contain "::")
        if (functionName.find("::") != std::string::npos) {
            return;
        }

        // Check if function is registered
        const auto* funcMetadata = ctx.program.getFunction(functionName);
        if (!funcMetadata) {
            return;
        }

        // Skip all validation for native functions (they handle their own parameter checking at runtime)
        if (funcMetadata->isNative) {
            return;
        }

        if (funcMetadata->parameterCount != arguments.size()) {
            throw errors::EnvironmentException(
                "Function '" + functionName + "' expects " +
                std::to_string(funcMetadata->parameterCount) +
                " parameter(s) but got " + std::to_string(arguments.size()),
                node->getLocation()
            );
        }

        // Validate parameter types
        if (funcMetadata->parameterTypes.empty()) {
            return;
        }

        for (size_t i = 0; i < arguments.size(); ++i) {
            std::string expectedType = funcMetadata->parameterTypes[i];

            // Resolve generic type parameters if present
            expectedType = ctx.resolveGenericType(expectedType);

            value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

            // Skip validation if type inference failed (returned void/unknown)
            if (argType == value::ValueType::VOID) {
                continue; // Can't validate - inference failed or expression is void
            }

            // Convert argType to string for comparison
            std::string argTypeStr = vm::runtime::utils::TypeConverter::valueTypeToString(argType);

            // For object types, need to check class names
            if (expectedType != "int" && expectedType != "float" &&
                expectedType != "string" && expectedType != "bool" &&
                expectedType != "void") {
                // Expected type is an object/class
                if (argType != value::ValueType::OBJECT) {
                    // null can be passed to object types
                    if (!dynamic_cast<ast::NullNode*>(arguments[i].get())) {
                        throw errors::TypeException(
                            "Function '" + functionName + "' parameter " + std::to_string(i + 1) +
                            " expects " + expectedType + " but got " + argTypeStr,
                            node->getLocation()
                        );
                    }
                }
                // For objects, check class name compatibility
                else if (expectedType != "object") {
                    std::string argClassName = ctx.typeInference.inferExpressionClassName(arguments[i].get());

                    // Normalize array types: convert "int[]" to "Array<int>", "int[][]" to "Array<Array<int>>", etc.
                    auto normalizeArrayType = [](const std::string& type) -> std::string {
                        std::string normalized = type;
                        size_t arrayDepth = 0;

                        // Count array brackets from the end
                        while (normalized.length() >= 2 && normalized.substr(normalized.length() - 2) == "[]") {
                            arrayDepth++;
                            normalized = normalized.substr(0, normalized.length() - 2);
                        }

                        // Wrap in Array<> for each dimension
                        for (size_t i = 0; i < arrayDepth; ++i) {
                            normalized = "Array<" + normalized + ">";
                        }

                        return normalized;
                    };

                    std::string normalizedArgClassName = normalizeArrayType(argClassName);
                    std::string normalizedExpectedType = normalizeArrayType(expectedType);

                    if (!argClassName.empty() && normalizedArgClassName != normalizedExpectedType) {
                        // Check if both are generic types with the same base
                        bool isGenericMatch = false;
                        size_t expectedAngle = normalizedExpectedType.find('<');
                        size_t argAngle = normalizedArgClassName.find('<');

                        if (expectedAngle != std::string::npos && argAngle != std::string::npos) {
                            // Both are generic types - check if base types match
                            std::string expectedBase = normalizedExpectedType.substr(0, expectedAngle);
                            std::string argBase = normalizedArgClassName.substr(0, argAngle);
                            if (expectedBase == argBase) {
                                // Same generic base type (e.g., both are List, both are Array)
                                // Type argument compatibility will be validated at runtime
                                isGenericMatch = true;
                            }
                        }

                        if (!isGenericMatch) {
                            // null can be passed to any object type
                            if (!dynamic_cast<ast::NullNode*>(arguments[i].get())) {
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
            // For primitive types, check exact match (with int->float exception)
            else if (expectedType != argTypeStr) {
                // Allow int to float conversion
                if (!(expectedType == "float" && argTypeStr == "int")) {
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
        if (actualFunctionName.substr(0, 6) == "this::" && ctx.currentClassNode) {
            std::string methodName = actualFunctionName.substr(6); // Remove "this::"
            actualFunctionName = ctx.currentClassNode->getClassName() + "::" + methodName;
        }

        // Validate static method exists at compile time
        if (ctx.compileTimeValidator) {
            // Extract className and methodName from qualified name
            size_t colonPos = actualFunctionName.find("::");
            if (colonPos != std::string::npos) {
                std::string className = actualFunctionName.substr(0, colonPos);
                std::string methodName = actualFunctionName.substr(colonPos + 2);
                ctx.compileTimeValidator->validateStaticMethodExists(className, methodName,
                                                                     arguments.size(), node->getLocation());
            }
        }

        // Compile all arguments (left to right)
        for (const auto& arg : arguments) {
            arg->accept(ctx.visitor);
        }

        // Note: We don't append "$static" here because the CALL_STATIC opcode handler
        // will append it when looking up the bytecode
        size_t nameIndex = ctx.program.getConstantPool().addString(actualFunctionName);
        // Static method call - use CALL_STATIC with source location
        ctx.program.emit(bytecode::OpCode::CALL_STATIC,
                     static_cast<uint32_t>(nameIndex),
                     static_cast<uint32_t>(arguments.size()));
        // Add source location for the call instruction
        ctx.program.addSourceLocation(ctx.program.getCurrentOffset() - 1,
                                     node->getLocation().getLine(),
                                     node->getLocation().getColumn(),
                                     node->getLocation().getFilename());
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

        for (const auto& method : methods) {
            if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get())) {
                if (methodNode->getName() == functionName &&
                    methodNode->getParameters().size() == arguments.size()) {
                    // If we're in a static method, only match static methods
                    // If we're in an instance method, prefer instance methods
                    if (ctx.inStaticMethod) {
                        if (methodNode->getIsStatic()) {
                            isMethodCall = true;
                            isStaticMethodCall = true;
                            break;
                        }
                    } else if (ctx.inInstanceMethod) {
                        if (!methodNode->getIsStatic()) {
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
        if (!isMethodCall && ctx.inInstanceMethod) {
            for (const auto& method : methods) {
                if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get())) {
                    if (methodNode->getName() == functionName &&
                        methodNode->getParameters().size() == arguments.size() &&
                        methodNode->getIsStatic()) {
                        isMethodCall = true;
                        isStaticMethodCall = true;
                        break;
                    }
                }
            }
        }

        if (isMethodCall) {
            if (isStaticMethodCall) {
                // Static method call - use CALL_STATIC with fully qualified name
                std::string qualifiedName = ctx.currentClassNode->getClassName() + "::" + functionName;

                // Validate static method exists at compile time
                if (ctx.compileTimeValidator) {
                    ctx.compileTimeValidator->validateStaticMethodExists(
                        ctx.currentClassNode->getClassName(), functionName,
                        arguments.size(), node->getLocation());
                }

                // Compile all arguments
                for (const auto& arg : arguments) {
                    arg->accept(ctx.visitor);
                }

                size_t nameIndex = ctx.program.getConstantPool().addString(qualifiedName);
                ctx.program.emit(bytecode::OpCode::CALL_STATIC,
                             static_cast<uint32_t>(nameIndex),
                             static_cast<uint32_t>(arguments.size()));
                ctx.program.addSourceLocation(ctx.program.getCurrentOffset() - 1,
                                             node->getLocation().getLine(),
                                             node->getLocation().getColumn(),
                                             node->getLocation().getFilename());
            } else {
                // Instance method call - push 'this' onto stack BEFORE arguments

                // Validate instance method exists at compile time
                if (ctx.compileTimeValidator) {
                    ctx.compileTimeValidator->validateInstanceMethodExists(
                        ctx.currentClassNode->getClassName(), functionName,
                        arguments.size(), node->getLocation());
                }

                ctx.program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(0));

                // Now compile arguments
                for (const auto& arg : arguments) {
                    arg->accept(ctx.visitor);
                }

                size_t nameIndex = ctx.program.getConstantPool().addString(functionName);
                // Call method on 'this' with source location
                ctx.program.emit(bytecode::OpCode::CALL_METHOD,
                             static_cast<uint32_t>(nameIndex),
                             static_cast<uint32_t>(arguments.size()));
                ctx.program.addSourceLocation(ctx.program.getCurrentOffset() - 1,
                                             node->getLocation().getLine(),
                                             node->getLocation().getColumn(),
                                             node->getLocation().getFilename());
            }
        } else {
            // Regular function call
            emitRegularFunctionCall(node, functionName, arguments);
        }
    }

    void FunctionCallHelper::emitRegularFunctionCall(ast::FunctionCallNode* node, const std::string& functionName,
                                                   const std::vector<std::unique_ptr<ast::ASTNode>>& arguments)
    {
        // Validate function exists at compile time (all functions are pre-registered)
        if (ctx.compileTimeValidator) {
            ctx.compileTimeValidator->validateFunctionExists(functionName, node->getLocation());
        }

        // Compile all arguments (left to right)
        for (const auto& arg : arguments) {
            arg->accept(ctx.visitor);
        }

        size_t nameIndex = ctx.program.getConstantPool().addString(functionName);
        // Regular function call - use CALL with source location
        ctx.program.emit(bytecode::OpCode::CALL,
                     static_cast<uint32_t>(nameIndex),
                     static_cast<uint32_t>(arguments.size()));
        // Add source location for the call instruction
        ctx.program.addSourceLocation(ctx.program.getCurrentOffset() - 1,
                                     node->getLocation().getLine(),
                                     node->getLocation().getColumn(),
                                     node->getLocation().getFilename());
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
        if (functionName.find("::") != std::string::npos) {
            // Static method call (ClassName::methodName)
            emitStaticMethodCall(node, functionName, arguments);
        } else if ((ctx.inInstanceMethod || ctx.inStaticMethod) && ctx.currentClassNode) {
            // Unqualified call inside a method - could be instance method, static method, or function
            emitMethodCallInClassContext(node, functionName, arguments);
        } else {
            // Regular function call
            emitRegularFunctionCall(node, functionName, arguments);
        }

        // Pop generic type bindings if we pushed them
        if (hasGenericBindings) {
            ctx.popGenericTypeBindings();
        }

        return std::monostate{};
    }
}
