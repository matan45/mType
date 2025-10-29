#include "FunctionCompiler.hpp"
#include "FunctionCallHelper.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../runtime/utils/TypeConverter.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../ast/nodes/functions/ReturnNode.hpp"
#include "../../../ast/nodes/expressions/LambdaNode.hpp"
#include  <iostream>
namespace vm::compiler::visitors
{
    FunctionCompiler::FunctionCompiler(CompilerContext& context)
        : ctx(context)
        , callHelper(std::make_unique<FunctionCallHelper>(context))
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
                paramTypes.push_back(vm::runtime::utils::TypeConverter::valueTypeToString(paramType.basicType));
            }
        }

        // Convert return type to string, preserving class names for object types
        std::string returnTypeStr;
        auto genericReturnType = node->getGenericReturnType();
        if (genericReturnType) {
            returnTypeStr = genericReturnType->toString();
        } else {
            returnTypeStr = vm::runtime::utils::TypeConverter::valueTypeToString(node->getReturnType());
        }

        // Enter function frame for local variable tracking
        ctx.functionFrameManager.enterFunctionFrame(returnTypeStr,
            ctx.variableTracker.getNextLocalSlot(),
            ctx.variableTracker.getCurrentScopeDepth(),
            false,  // Not a lambda
            node->getIsAsync());  // Mark if async function
        ctx.variableTracker.beginScope();  // Function body scope

        // For generic functions, push empty bindings (will be filled during call)
        // This ensures the function body can be compiled even with generic types
        bool pushedGenericBindings = false;
        if (node->isGeneric())
        {
            std::unordered_map<std::string, std::string> emptyBindings;
            const auto& genericParams = node->getGenericTypeParameters();
            // Map each generic type to itself for now (will be resolved at call time)
            for (const auto& param : genericParams)
            {
                emptyBindings[param.name] = param.name;
            }
            ctx.pushGenericTypeBindings(emptyBindings);
            pushedGenericBindings = true;
        }

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

        // Pop generic bindings if we pushed them
        if (pushedGenericBindings)
        {
            ctx.popGenericTypeBindings();
        }

        // Emit implicit return for void functions (if no explicit return)
        // This includes both:
        // - function foo(): void { ... }
        // - function async foo(): Promise<void> { ... }
        bool isVoidFunction = (node->getReturnType() == value::ValueType::VOID);
        bool isAsyncVoidFunction = (node->getIsAsync() && returnTypeStr == "Promise<void>");

        if (isVoidFunction || isAsyncVoidFunction) {
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
            // Wrap in Promise if async function
            if (node->getIsAsync()) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }
            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
        }

        // Calculate local count before exiting frame
        size_t localCount = ctx.functionFrameManager.getLocalCount();

        // Capture local variable names for debugging (before exiting frame)
        const auto& locals = ctx.variableTracker.getLocals();
        const auto& currentFrame = ctx.functionFrameManager.currentFrame();

        std::vector<std::string> localVarNames(localCount);
        for (const auto& local : locals)
        {
            // Convert absolute slot to relative slot within this function
            if (local.slot >= currentFrame.localStartSlot)
            {
                size_t relativeSlot = local.slot - currentFrame.localStartSlot;
                if (relativeSlot < localCount)
                {
                    localVarNames[relativeSlot] = local.name;
                }
            }
        }

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
        metadata.isAsync = node->getIsAsync();  // NEW: Copy async flag from AST
        metadata.localVariableNames = localVarNames;  // NEW: Store variable names for debugging

        // Store generic type parameters if the function is generic
        if (node->isGeneric())
        {
            const auto& genericParams = node->getGenericTypeParameters();
            for (const auto& param : genericParams)
            {
                metadata.genericTypeParameters.push_back(param.name);
            }
        }

        ctx.program.registerFunction(funcName, metadata);

        return std::monostate{};
    }

    value::Value FunctionCompiler::compileFunctionCall(ast::FunctionCallNode* node)
    {
        return callHelper->compileFunctionCall(node);
    }

    void FunctionCompiler::validateReturnType(ast::ReturnNode* node, ast::ASTNode* returnValue)
    {
        if (!ctx.functionFrameManager.isInFunction()) {
            return; // Not in a function context
        }

        std::string expectedReturnType = ctx.functionFrameManager.currentFrame().returnType;

        // Skip validation for "auto" return type (type inference)
        if (expectedReturnType == "auto") {
            return;
        }

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
                        // Special case: int can be returned for float
                        if (!(expectedType == value::ValueType::FLOAT && actualType == value::ValueType::INT)) {
                            std::string actualTypeStr = vm::runtime::utils::TypeConverter::valueTypeToString(actualType);
                            throw errors::TypeException(
                                "Return type mismatch: expected " + expectedReturnType + " but got " + actualTypeStr,
                                node->getLocation()
                            );
                        }
                    }
                }
                // For OBJECT types, validate class compatibility
                else if (expectedType == value::ValueType::OBJECT && actualType == value::ValueType::OBJECT) {
                    std::string actualClassName = ctx.typeInference.inferExpressionClassName(returnValue);

                    // Skip validation for generic array types (like Array<T>, T[], etc.)
                    bool isGenericArrayReturn = (expectedReturnType == "Array" || expectedReturnType.find("Array<") == 0);
                    bool isConcreteArrayReturn = actualClassName.find("[]") != std::string::npos;

                    if (!(isGenericArrayReturn && isConcreteArrayReturn)) {
                        // Use TypeValidator for detailed class compatibility checking
                        if (!actualClassName.empty() && expectedReturnType != "object") {
                            bool isNullValue = dynamic_cast<ast::NullNode*>(returnValue) != nullptr;
                            ctx.typeValidator.validateAssignment(
                                expectedType, expectedReturnType,
                                actualType, actualClassName,
                                isNullValue, node->getLocation()
                            );
                        }
                    }
                }
            }
        } else {
            // Function returns nothing (return;)
            // For async functions, allow return; in Promise<void> functions
            bool isAsyncVoidReturn = ctx.functionFrameManager.currentFrame().isAsync &&
                                     expectedReturnType == "Promise<void>";

            if (expectedReturnType != "void" && !isAsyncVoidReturn) {
                throw errors::TypeException(
                    "Return type mismatch: expected " + expectedReturnType + " but got void",
                    node->getLocation()
                );
            }
        }
    }

    void FunctionCompiler::emitReturnWithFinally(ast::ReturnNode* node, ast::ASTNode* returnValue)
    {
        // Check if we already have a return value slot for this try-finally block
        // IMPORTANT: Reuse the same slot for all returns in the same try-finally to ensure
        // the finally block loads the correct return value
        size_t returnValueSlot = ctx.exceptionManager.getReturnValueSlot();
        if (returnValueSlot == SIZE_MAX) {
            // First return in this try block - allocate a new slot
            returnValueSlot = ctx.variableTracker.getNextLocalSlot();
            ctx.functionFrameManager.updateMaxLocalSlot(returnValueSlot + 1);

            // Remember this slot so ControlFlowCompiler can load it back after finally
            ctx.exceptionManager.setReturnValueSlot(returnValueSlot);
        }

        if (returnValue) {
            // For async functions, wrap in Promise before storing
            // This ensures consistency - we always store a Promise for async functions
            if (ctx.functionFrameManager.currentFrame().isAsync) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            // Store return value in the special slot
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(returnValueSlot), node);
        } else {
            // Push null for void return
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);

            // For async functions, wrap in Promise before storing for finally
            if (ctx.functionFrameManager.currentFrame().isAsync) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            // Store return value
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(returnValueSlot), node);
        }

        // Jump to finally
        size_t returnJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
        ctx.exceptionManager.registerReturnJump(returnJump);
    }

    void FunctionCompiler::emitReturnWithOuterFinally(ast::ReturnNode* node, ast::ASTNode* returnValue)
    {
        // We're in a finally block that has a return, but there's an outer finally that must execute
        // Get or create the outer context's return value slot
        size_t outerReturnValueSlot = ctx.exceptionManager.getReturnValueSlotForOuter();
        if (outerReturnValueSlot == SIZE_MAX) {
            outerReturnValueSlot = ctx.variableTracker.getNextLocalSlot();
            ctx.functionFrameManager.updateMaxLocalSlot(outerReturnValueSlot + 1);
            ctx.exceptionManager.setReturnValueSlotForOuter(outerReturnValueSlot);
        }

        if (returnValue) {
            // Wrap in Promise if needed
            if (ctx.functionFrameManager.currentFrame().isAsync) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }
            // Store return value in outer slot
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(outerReturnValueSlot), node);
        } else {
            // Push null for void return
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);

            // For async functions, wrap in Promise
            if (ctx.functionFrameManager.currentFrame().isAsync) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            // Store return value
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(outerReturnValueSlot), node);
        }

        // Jump to outer finally
        size_t returnJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
        ctx.exceptionManager.registerReturnJumpWithOuter(returnJump);
    }

    void FunctionCompiler::emitReturnValueBytecode(ast::ReturnNode* node, ast::ASTNode* returnValue)
    {
        // Use returnValue's location if available (fallback for when ReturnNode lacks proper location)
        ast::ASTNode* locationNode = (returnValue && returnValue->getLocation().getLine() > 0) ? returnValue : node;

        if (returnValue) {
            // Wrap in Promise if needed and return immediately
            if (ctx.functionFrameManager.currentFrame().isAsync) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, locationNode);
            }
            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, locationNode);
        } else {
            // For async functions, treat return; as return null; wrapped in Promise
            // This allows Promise<void> functions to use return; like regular void functions
            if (ctx.functionFrameManager.currentFrame().isAsync) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            } else {
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN, node);
            }
        }
    }

    value::Value FunctionCompiler::compileReturn(ast::ReturnNode* node)
    {
        auto* returnValue = node->getReturnValue();

        // Validate return type matches function signature
        validateReturnType(node, returnValue);

        // Compile return value expression if present
        if (returnValue) {
            returnValue->accept(ctx.visitor);
        }

        // Check if in function context
        if (ctx.functionFrameManager.isInFunction()) {
            // Check if we're in a try block with a finally, but NOT already inside the finally block
            // If we're IN the finally block, we should return directly (not jump to finally again)
            if (ctx.exceptionManager.hasPendingFinally() && !ctx.exceptionManager.isInFinally()) {
                emitReturnWithFinally(node, returnValue);
            } else if (ctx.exceptionManager.isInFinally() && ctx.exceptionManager.hasOuterFinally()) {
                // We're in a finally block, but there's an outer finally that must execute
                // Store return value and jump to outer finally instead of returning immediately
                emitReturnWithOuterFinally(node, returnValue);
            } else {
                emitReturnValueBytecode(node, returnValue);
            }
        } else {
            // Not in a function context - fallback handling
            if (returnValue) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            } else {
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN, node);
            }
        }

        return std::monostate{};
    }

    std::vector<variables::VariableTracker::LocalVariable> FunctionCompiler::captureScopeVariables()
    {
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

        return capturedVars;
    }

    void FunctionCompiler::setupLambdaFrame(ast::LambdaNode* node,
                                           const std::vector<variables::VariableTracker::LocalVariable>& capturedVars)
    {
        const auto& params = node->getParameters();

        // Enter function frame for lambda
        ctx.functionFrameManager.enterFunctionFrame("auto",
            0,  // Lambda parameters start from slot 0
            ctx.variableTracker.getCurrentScopeDepth(),
            true,  // Mark this frame as a lambda
            node->getIsAsync());  // Mark if async lambda
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
    }

    void FunctionCompiler::emitLambdaInstruction(size_t lambdaStart, ast::LambdaNode* node,
                                                 const std::vector<variables::VariableTracker::LocalVariable>& capturedVars,
                                                 size_t currentFrameStart,
                                                 const std::vector<variables::VariableTracker::LocalVariable>& currentLocals)
    {
        const auto& params = node->getParameters();

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

        ctx.emitter.emitWithLocation(bytecode::OpCode::LAMBDA, operands, node);
    }

    value::Value FunctionCompiler::compileLambda(ast::LambdaNode* node)
    {
        // Generate unique lambda function name
        static size_t lambdaCounter = 0;
        std::string lambdaFuncName = "__lambda_" + std::to_string(lambdaCounter++);

        // Store current position to jump over lambda body
        ctx.emitter.emitWithLocation(bytecode::OpCode::JUMP, 0u, node); // Placeholder
        size_t skipJump = ctx.program.getCurrentOffset() - 1;

        // Lambda function starts here
        size_t lambdaStart = ctx.program.getCurrentOffset();

        // Save current local slot state
        size_t savedNextLocalSlot = ctx.variableTracker.getNextLocalSlot();

        // Capture variables from outer scope for closure support
        const auto& currentLocals = ctx.variableTracker.getLocals();
        auto capturedVars = captureScopeVariables();
        size_t currentFrameStart = ctx.functionFrameManager.isInFunction() ?
            ctx.functionFrameManager.currentFrame().localStartSlot : 0;

        // Reset local slot counter for lambda's own scope
        ctx.variableTracker.resetLocalSlot();

        // Setup lambda frame with parameters and captured variables
        setupLambdaFrame(node, capturedVars);

        // Compile lambda body
        auto* body = node->getBody();
        if (node->isExpressionLambda()) {
            // Expression lambda: () -> expr
            body->accept(ctx.visitor);

            // Wrap in Promise if async lambda
            if (node->getIsAsync()) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
        }
        else {
            // Block lambda: () -> { ... }
            body->accept(ctx.visitor);

            // Implicit return null if no explicit return
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);

            // Wrap in Promise if async lambda
            if (node->getIsAsync()) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
        }

        // Capture local variable names for debugging (before exiting frame)
        const auto& locals = ctx.variableTracker.getLocals();
        const auto& currentFrame = ctx.functionFrameManager.currentFrame();
        size_t localCount = node->getParameters().size() + capturedVars.size();
        std::vector<std::string> localVarNames(localCount);
        for (const auto& local : locals)
        {
            // Convert absolute slot to relative slot within this lambda
            if (local.slot >= currentFrame.localStartSlot)
            {
                size_t relativeSlot = local.slot - currentFrame.localStartSlot;
                if (relativeSlot < localCount)
                {
                    localVarNames[relativeSlot] = local.name;
                }
            }
        }

        ctx.variableTracker.endScope();
        ctx.functionFrameManager.exitFunctionFrame();

        // Lambda function ends here
        size_t lambdaEnd = ctx.program.getCurrentOffset();

        // Patch the skip jump to here
        ctx.program.patchJump(skipJump, static_cast<uint32_t>(lambdaEnd));

        // Register lambda metadata for peephole optimizer
        // This ensures lambda offsets are updated when instructions are removed
        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = lambdaFuncName;
        metadata.startOffset = lambdaStart;
        metadata.instructionCount = lambdaEnd - lambdaStart;
        metadata.localCount = localCount;
        metadata.parameterCount = node->getParameters().size();
        metadata.returnType = "auto";  // Lambda return type is inferred
        metadata.isNative = false;
        metadata.isAsync = node->getIsAsync();
        metadata.localVariableNames = localVarNames;
        ctx.program.registerFunction(lambdaFuncName, metadata);

        // Emit lambda instruction with captured environment
        emitLambdaInstruction(lambdaStart, node, capturedVars, currentFrameStart, currentLocals);

        // Restore previous nextLocalSlot
        ctx.variableTracker.setLocalSlot(savedNextLocalSlot);

        return std::monostate{};
    }
}
