#include "FunctionCompiler.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../evaluator/utils/ValueConverter.hpp"
#include <iostream>
namespace vm::compiler::visitors
{
    FunctionCompiler::FunctionCompiler(CompilerContext& context)
        : ctx(context)
    {
    }

    value::Value FunctionCompiler::compileFunction(ast::FunctionNode* node)
    {
        std::string funcName = node->getName();

        // Check if this function is already registered as a native function
        const auto* existingFunc = ctx.program.getFunction(funcName);
        if (existingFunc && existingFunc->isNative) {
            // Skip compilation - native functions are implemented in C++ and already registered
            return std::monostate{};
        }

        // Emit JUMP to skip over function body during main execution
        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Function starts here
        size_t functionStart = ctx.program.getCurrentOffset();

        // Get parameters with type information preserved
        auto paramTypesVec = node->getParameterTypes();
        std::vector<std::string> paramNames;
        std::vector<std::string> paramTypes;
        for (const auto& param : paramTypesVec) {
            paramNames.push_back(param.first);
            // ParameterType preserves class names for object types
            const auto& paramType = param.second;
            if (paramType.basicType == value::ValueType::OBJECT && paramType.className.has_value()) {
                paramTypes.push_back(paramType.className.value());
            } else {
                paramTypes.push_back(evaluator::utils::ValueConverter::valueTypeToString(paramType.basicType));
            }
        }

        // Convert return type to string, preserving class names for object types
        std::string returnTypeStr;
        auto genericReturnType = node->getGenericReturnType();
        if (genericReturnType) {
            returnTypeStr = genericReturnType->toString();
        } else {
            returnTypeStr = evaluator::utils::ValueConverter::valueTypeToString(node->getReturnType());
        }

        // Enter function frame for local variable tracking
        ctx.functionFrameManager.enterFunctionFrame(returnTypeStr,
            ctx.variableTracker.getNextLocalSlot(),
            ctx.variableTracker.getCurrentScopeDepth(),
            false);
        ctx.variableTracker.beginScope();  // Function body scope

        // Track parameters as locals
        for (const auto& param : paramTypesVec) {
            ctx.variableTracker.declareLocal(param.first, param.second.basicType,
                param.second.className.value_or(""));
        }

        // Update max local slot after parameters
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        // Compile function body
        auto* body = node->getBodyPtr();
        if (body) {
            body->accept(ctx.visitor);  // Will need delegation
        }

        // Emit implicit return for void functions (if no explicit return)
        if (node->getReturnType() == value::ValueType::VOID) {
            ctx.program.emit(bytecode::OpCode::PUSH_NULL);
            ctx.program.emit(bytecode::OpCode::RETURN_VALUE);
        }

        // Calculate local count before exiting frame
        size_t localCount = ctx.functionFrameManager.getLocalCount();

        ctx.variableTracker.endScope();      // End function body scope
        ctx.functionFrameManager.exitFunctionFrame();

        size_t functionEnd = ctx.program.getCurrentOffset();

        // Patch skip jump to here (after function)
        ctx.emitter.patchJump(skipJump);

        // Register function metadata
        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = funcName;
        metadata.startOffset = functionStart;
        metadata.instructionCount = functionEnd - functionStart;
        metadata.localCount = localCount;
        metadata.parameterCount = paramTypesVec.size();
        metadata.parameterNames = paramNames;
        metadata.parameterTypes = paramTypes;
        metadata.returnType = returnTypeStr;
        metadata.isNative = false;

        ctx.program.registerFunction(funcName, metadata);

        return std::monostate{};
    }

    value::Value FunctionCompiler::compileFunctionCall(ast::FunctionCallNode* node)
    {
        // Get function name
        std::string functionName = node->getFunctionName();
        const auto& arguments = node->getArguments();

        // Validate parameter count and types for non-method calls
        if (functionName.find("::") == std::string::npos) {
            // Check if function is registered
            const auto* funcMetadata = ctx.program.getFunction(functionName);
            if (funcMetadata) {
                // Skip all validation for native functions (they handle their own parameter checking at runtime)
                if (!funcMetadata->isNative) {
                    if (funcMetadata->parameterCount != arguments.size()) {
                        throw errors::EnvironmentException(
                            "Function '" + functionName + "' expects " +
                            std::to_string(funcMetadata->parameterCount) +
                            " parameter(s) but got " + std::to_string(arguments.size()),
                            node->getLocation()
                        );
                    }

                    // Validate parameter types
                    if (!funcMetadata->parameterTypes.empty()) {
                        for (size_t i = 0; i < arguments.size(); ++i) {
                            std::string expectedType = funcMetadata->parameterTypes[i];
                            value::ValueType argType = ctx.typeInference.inferExpressionType(arguments[i].get());

                            // Convert argType to string for comparison
                            std::string argTypeStr = evaluator::utils::ValueConverter::valueTypeToString(argType);

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
                                    if (!argClassName.empty() && argClassName != expectedType) {
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
                }
            }
        }

        // Check if this is a static method call (ClassName::methodName)
        if (functionName.find("::") != std::string::npos) {
            // Replace "this::" with actual class name if inside a class
            if (functionName.substr(0, 6) == "this::" && ctx.currentClassNode) {
                std::string methodName = functionName.substr(6); // Remove "this::"
                functionName = ctx.currentClassNode->getClassName() + "::" + methodName;
            }

            // Compile all arguments (left to right)
            for (const auto& arg : arguments) {
                arg->accept(ctx.visitor);  // Will need delegation
            }

            size_t nameIndex = ctx.program.getConstantPool().addString(functionName);
            // Static method call - use CALL_STATIC
            ctx.program.emit(bytecode::OpCode::CALL_STATIC,
                         static_cast<uint32_t>(nameIndex),
                         static_cast<uint32_t>(arguments.size()));
        } else if (ctx.inInstanceMethod && ctx.currentClassNode) {
            // Unqualified call inside an instance method - could be either:
            // 1. Method call on 'this' (recursive or calling another method)
            // 2. Regular function call (global function)
            //
            // Check if a method with this name exists in the current class
            bool isMethodCall = false;
            const auto& methods = ctx.currentClassNode->getMethods();

            for (const auto& method : methods) {
                if (auto* methodNode = dynamic_cast<ast::MethodNode*>(method.get())) {
                    if (methodNode->getName() == functionName &&
                        methodNode->getParameters().size() == arguments.size()) {
                        isMethodCall = true;
                        break;
                    }
                }
            }

            if (isMethodCall) {
                // Push 'this' onto stack BEFORE arguments
                ctx.program.emit(bytecode::OpCode::LOAD_LOCAL, static_cast<uint32_t>(0));

                // Now compile arguments
                for (const auto& arg : arguments) {
                    arg->accept(ctx.visitor);  // Will need delegation
                }

                size_t nameIndex = ctx.program.getConstantPool().addString(functionName);
                // Call method on 'this'
                ctx.program.emit(bytecode::OpCode::CALL_METHOD,
                             static_cast<uint32_t>(nameIndex),
                             static_cast<uint32_t>(arguments.size()));
            } else {
                // Regular function call
                for (const auto& arg : arguments) {
                    arg->accept(ctx.visitor);  // Will need delegation
                }

                size_t nameIndex = ctx.program.getConstantPool().addString(functionName);
                ctx.program.emit(bytecode::OpCode::CALL,
                             static_cast<uint32_t>(nameIndex),
                             static_cast<uint32_t>(arguments.size()));
            }
        } else {
            // Compile all arguments (left to right)
            for (const auto& arg : arguments) {
                arg->accept(ctx.visitor);  // Will need delegation
            }

            size_t nameIndex = ctx.program.getConstantPool().addString(functionName);
            // Regular function call - use CALL
            ctx.program.emit(bytecode::OpCode::CALL,
                         static_cast<uint32_t>(nameIndex),
                         static_cast<uint32_t>(arguments.size()));
        }

        return std::monostate{};
    }

    value::Value FunctionCompiler::compileReturn(ast::ReturnNode* node)
    {
        auto* returnValue = node->getReturnValue();

        // Type validation: check return type matches function signature
        if (ctx.functionFrameManager.isInFunction()) {
            std::string expectedReturnType = ctx.functionFrameManager.currentFrame().returnType;

            // Skip validation for "auto" return type (type inference)
            if (expectedReturnType != "auto") {
                if (returnValue) {
                    // Function has a return value
                    value::ValueType actualType = ctx.typeInference.inferExpressionType(returnValue);

                    // Convert expected return type string to ValueType
                    value::ValueType expectedType = value::ValueType::VOID;
                    if (expectedReturnType == "int") expectedType = value::ValueType::INT;
                    else if (expectedReturnType == "float") expectedType = value::ValueType::FLOAT;
                    else if (expectedReturnType == "string") expectedType = value::ValueType::STRING;
                    else if (expectedReturnType == "bool") expectedType = value::ValueType::BOOL;
                    else if (expectedReturnType == "void") expectedType = value::ValueType::VOID;
                    else expectedType = value::ValueType::OBJECT; // Class/interface types

                    // Check if types match (allow VOID for unknown types)
                    if (actualType != value::ValueType::VOID && expectedType != value::ValueType::VOID) {
                        if (actualType != expectedType) {
                            // Special case: null can be returned for object types
                            if (!(expectedType == value::ValueType::OBJECT && dynamic_cast<ast::NullNode*>(returnValue))) {
                                std::string actualTypeStr = evaluator::utils::ValueConverter::valueTypeToString(actualType);
                                throw errors::TypeException(
                                    "Return type mismatch: expected " + expectedReturnType + " but got " + actualTypeStr,
                                    node->getLocation()
                                );
                            }
                        }
                    }
                } else {
                    // Function returns nothing
                    if (expectedReturnType != "void") {
                        throw errors::TypeException(
                            "Return type mismatch: expected " + expectedReturnType + " but got void",
                            node->getLocation()
                        );
                    }
                }
            }

            if (returnValue) {
                returnValue->accept(ctx.visitor);  // Will need delegation
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            } else {
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN, node);
            }
        } else {
            // Not in a function context - shouldn't happen but handle gracefully
            if (returnValue) {
                returnValue->accept(ctx.visitor);  // Will need delegation
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            } else {
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN, node);
            }
        }

        return std::monostate{};
    }

    value::Value FunctionCompiler::compileNativeFunction(ast::NativeFunctionNode* node)
    {
        // Native function declarations are already registered at the start of compilation
        // from the environment's native registry, so we just skip them here
        return std::monostate{};
    }

    value::Value FunctionCompiler::compileLambda(ast::LambdaNode* node)
    {
        // Generate unique lambda function name
        static size_t lambdaCounter = 0;
        std::string lambdaFuncName = "__lambda_" + std::to_string(lambdaCounter++);

        // Store current position to jump over lambda body
        ctx.program.emit(bytecode::OpCode::JUMP, 0); // Placeholder
        size_t skipJump = ctx.program.getCurrentOffset() - 1;

        // Lambda function starts here
        size_t lambdaStart = ctx.program.getCurrentOffset();

        // Get parameters
        const auto& params = node->getParameters();

        // Save current local slot state
        size_t savedNextLocalSlot = ctx.variableTracker.getNextLocalSlot();

        // Capture variables from outer scope for closure support
        std::vector<variables::VariableTracker::LocalVariable> capturedVars;
        const auto& currentLocals = ctx.variableTracker.getLocals();

        // Determine if we're compiling a lambda inside another lambda
        bool isNestedLambda = ctx.functionFrameManager.isInFunction() &&
                              ctx.functionFrameManager.currentFrame().isLambda;

        // Capture strategy: capture all variables from the current function frame
        size_t currentFrameStart = ctx.functionFrameManager.isInFunction() ?
            ctx.functionFrameManager.currentFrame().localStartSlot : 0;

        for (const auto& local : currentLocals) {
            if (local.slot >= currentFrameStart) {
                capturedVars.push_back(local);
            }
        }

        // Reset local slot counter for lambda's own scope
        ctx.variableTracker.resetLocalSlot();

        // Enter function frame for lambda
        ctx.functionFrameManager.enterFunctionFrame("auto",
            0,  // Lambda parameters start from slot 0
            ctx.variableTracker.getCurrentScopeDepth(),
            true);  // Mark this frame as a lambda
        ctx.variableTracker.beginScope();

        // Track lambda parameters as locals (they occupy slots 0, 1, 2, ...)
        for (const auto& param : params) {
            ctx.variableTracker.declareLocal(param.name, value::ValueType::VOID, "");
        }

        // Add captured variables as locals (they occupy slots after parameters)
        for (const auto& capture : capturedVars) {
            ctx.variableTracker.declareLocal(capture.name, capture.type, capture.className);
        }

        // Update max local slot
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        // Compile lambda body
        auto* body = node->getBody();
        if (node->isExpressionLambda()) {
            // Expression lambda: () -> expr
            // Compile expression and return its value
            body->accept(ctx.visitor);  // Will need delegation
            ctx.program.emit(bytecode::OpCode::RETURN_VALUE);
        }
        else {
            // Block lambda: () -> { ... }
            // Compile block
            body->accept(ctx.visitor);  // Will need delegation

            // Implicit return null if no explicit return
            ctx.program.emit(bytecode::OpCode::PUSH_NULL);
            ctx.program.emit(bytecode::OpCode::RETURN_VALUE);
        }

        ctx.variableTracker.endScope();
        ctx.functionFrameManager.exitFunctionFrame();

        // Lambda function ends here
        size_t lambdaEnd = ctx.program.getCurrentOffset();

        // Patch the skip jump to here
        ctx.program.patchJump(skipJump, static_cast<uint32_t>(lambdaEnd));

        // Now emit instruction to create lambda value with captured environment
        std::vector<uint32_t> operands;
        operands.push_back(static_cast<uint32_t>(lambdaStart));
        operands.push_back(static_cast<uint32_t>(params.size()));
        operands.push_back(static_cast<uint32_t>(capturedVars.size()));
        operands.push_back(static_cast<uint32_t>(currentLocals.size()));  // Number of parent locals

        // Add captured variable slot numbers
        for (const auto& capture : capturedVars) {
            size_t relativeSlot = capture.slot - currentFrameStart;
            operands.push_back(static_cast<uint32_t>(relativeSlot));
        }

        // Add parent local variable name->slot mapping for late-bound access
        for (const auto& local : currentLocals) {
            size_t nameIndex = ctx.program.getConstantPool().addString(local.name);
            operands.push_back(static_cast<uint32_t>(nameIndex));
            operands.push_back(static_cast<uint32_t>(local.slot));
        }

        ctx.program.emit(bytecode::OpCode::LAMBDA, operands);

        // Restore previous nextLocalSlot
        ctx.variableTracker.setLocalSlot(savedNextLocalSlot);

        return std::monostate{};
    }
}
