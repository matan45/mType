#include "FunctionCompiler.hpp"
#include "FunctionCallHelper.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/EnvironmentException.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
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
        if (existingFunc && existingFunc->isNative)
        {
            // Skip compilation - native functions are implemented in C++ and already registered
            return std::monostate{};
        }

        // Emit JUMP to skip over function body during main execution
        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Function starts here
        size_t functionStart = ctx.program.getCurrentOffset();

        // Build list of valid generic type parameter names for validation
        std::vector<std::string> validGenericParams;
        if (node->isGeneric())
        {
            for (const auto& param : node->getGenericTypeParameters())
            {
                validGenericParams.push_back(param.name);
            }
        }

        // Get parameters with type information preserved
        auto paramTypesVec = node->getParameterTypes();
        std::vector<std::string> paramNames;
        std::vector<std::string> paramTypes;
        for (const auto& param : paramTypesVec)
        {
            paramNames.push_back(param.first);
            // ParameterType preserves class names for object types
            const auto& paramType = param.second;
            std::string paramTypeStr;
            if (paramType.basicType == value::ValueType::OBJECT && paramType.className.has_value())
            {
                paramTypeStr = paramType.className.value();
            }
            else
            {
                paramTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(paramType.basicType);
            }
            paramTypes.push_back(paramTypeStr);

            // Validate parameter type exists
            if (!isValidTypeName(paramTypeStr, validGenericParams))
            {
                throw errors::TypeException(
                    "Undefined type '" + paramTypeStr + "' in parameter '" + param.first + "'. " +
                    "Type must be a primitive, declared generic parameter, or existing class/interface.",
                    node->getLocation()
                );
            }
        }

        // Convert return type to string, preserving class names for object types
        std::string returnTypeStr;
        auto genericReturnType = node->getGenericReturnType();
        if (genericReturnType)
        {
            returnTypeStr = genericReturnType->toString();
        }
        else
        {
            returnTypeStr = ::types::TypeConversionUtils::getTypeDisplayName(node->getReturnType());
        }

        // Validate return type exists
        if (!isValidTypeName(returnTypeStr, validGenericParams))
        {
            throw errors::TypeException(
                "Undefined type '" + returnTypeStr + "' in return type. " +
                "Type must be a primitive, declared generic parameter, or existing class/interface.",
                node->getLocation()
            );
        }

        // Enter function frame for local variable tracking
        ctx.functionFrameManager.enterFunctionFrame(returnTypeStr,
                                                    ctx.variableTracker.getNextLocalSlot(),
                                                    ctx.variableTracker.getCurrentScopeDepth(),
                                                    false, // Not a lambda
                                                    node->getIsAsync()); // Mark if async function
        ctx.variableTracker.beginScope(); // Function body scope

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
        for (const auto& param : paramTypesVec)
        {
            ctx.variableTracker.declareLocal(param.first, param.second.basicType,
                                             param.second.className.value_or(""));
        }

        // Update max local slot after parameters
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        // Compile function body
        auto* body = node->getBodyPtr();
        if (body)
        {
            body->accept(ctx.visitor); // Will need delegation
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

        if (isVoidFunction || isAsyncVoidFunction)
        {
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
            // Wrap in Promise if async function
            if (node->getIsAsync())
            {
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

        ctx.variableTracker.endScope(); // End function body scope
        ctx.functionFrameManager.exitFunctionFrame();

        size_t functionEnd = ctx.program.getCurrentOffset();

        // Patch skip jump to here (after function)
        ctx.emitter.patchJump(skipJump);

        // PHASE 2 FIX: Get existing metadata to preserve parameterTypeParameterUsage
        // The function was already registered during FunctionRegistrar phase with this data
        const auto* existingMetadata = ctx.program.getFunction(funcName);

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
        metadata.isAsync = node->getIsAsync(); // NEW: Copy async flag from AST
        metadata.localVariableNames = localVarNames; // NEW: Store variable names for debugging

        // Store generic type parameters if the function is generic
        if (node->isGeneric())
        {
            const auto& genericParams = node->getGenericTypeParameters();
            for (const auto& param : genericParams)
            {
                metadata.genericTypeParameters.push_back(param.name);
            }

            // PHASE 2 FIX: Preserve parameterTypeParameterUsage from initial registration
            if (existingMetadata)
            {
                metadata.parameterTypeParameterUsage = existingMetadata->parameterTypeParameterUsage;
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
        if (!ctx.functionFrameManager.isInFunction())
        {
            return; // Not in a function context
        }

        std::string expectedReturnType = ctx.functionFrameManager.currentFrame().returnType;

        // Skip validation for "auto" return type (type inference)
        if (expectedReturnType == "auto")
        {
            return;
        }

        if (returnValue)
        {
            // Function has a return value
            value::ValueType actualType = ctx.typeInference.inferExpressionType(returnValue);

            // Convert expected return type string to ValueType
            value::ValueType expectedType = value::ValueType::VOID;
            if (expectedReturnType == "int") expectedType = value::ValueType::INT;
            else if (expectedReturnType == "float") expectedType = value::ValueType::FLOAT;
            else if (expectedReturnType == "string") expectedType = value::ValueType::STRING;
            else if (expectedReturnType == "bool") expectedType = value::ValueType::BOOL;
            else if (expectedReturnType == "void") expectedType = value::ValueType::VOID;
            else if (expectedReturnType.find("Array<") == 0 || expectedReturnType.find("[]") != std::string::npos) {
                expectedType = value::ValueType::ARRAY; // Array types (Array<T> or T[])
            }
            // PHASE 4: Handle Promise<Array<T>> or Promise<T[]> for async functions
            else if (expectedReturnType.find("Promise<") == 0) {
                // Extract inner type from Promise<T>
                size_t start = expectedReturnType.find('<') + 1;
                size_t end = expectedReturnType.rfind('>');
                if (start != std::string::npos && end != std::string::npos && end > start) {
                    std::string innerType = expectedReturnType.substr(start, end - start);
                    // Trim whitespace
                    innerType.erase(0, innerType.find_first_not_of(" \t"));
                    innerType.erase(innerType.find_last_not_of(" \t") + 1);

                    // Check if inner type is an array
                    if (innerType.find("Array<") == 0 || innerType.find("[]") != std::string::npos) {
                        expectedType = value::ValueType::ARRAY;
                    }
                    else {
                        expectedType = value::ValueType::OBJECT; // Promise<Object>
                    }
                }
                else {
                    expectedType = value::ValueType::OBJECT;
                }
            }
            else expectedType = value::ValueType::OBJECT; // Class/interface types

            // Check if types match (allow VOID for unknown types)
            if (actualType != value::ValueType::VOID && expectedType != value::ValueType::VOID)
            {
                if (actualType != expectedType)
                {
                    // Special case: null can be returned for object types
                    if (!(expectedType == value::ValueType::OBJECT && dynamic_cast<ast::NullNode*>(returnValue)))
                    {
                        // Special case: int can be returned for float
                        if (!(expectedType == value::ValueType::FLOAT && actualType == value::ValueType::INT))
                        {
                            // PHASE 4: Allow primitive literals when returning Box types (auto-boxing)
                            bool canAutoBox = false;
                            if (expectedType == value::ValueType::OBJECT)
                            {
                                // Check if expected is a Box type and actual is corresponding primitive
                                if ((expectedReturnType == "Int" && actualType == value::ValueType::INT) ||
                                    (expectedReturnType == "Float" && actualType == value::ValueType::FLOAT) ||
                                    (expectedReturnType == "Bool" && actualType == value::ValueType::BOOL) ||
                                    (expectedReturnType == "String" && actualType == value::ValueType::STRING))
                                {
                                    canAutoBox = true;
                                }
                            }

                            if (!canAutoBox)
                            {
                                std::string actualTypeStr =
                                    ::types::TypeConversionUtils::getTypeDisplayName(actualType);
                                throw errors::TypeException(
                                    "Return type mismatch: expected " + expectedReturnType + " but got " + actualTypeStr,
                                    node->getLocation()
                                );
                            }
                        }
                    }
                }
                // For OBJECT types, validate class compatibility
                else if (expectedType == value::ValueType::OBJECT && actualType == value::ValueType::OBJECT)
                {
                    std::string actualClassName = ctx.typeInference.inferExpressionClassName(returnValue);

                    // Skip validation for generic array types (like Array<T>, T[], etc.)
                    bool isGenericArrayReturn = (expectedReturnType == "Array" || expectedReturnType.find("Array<") ==
                        0);
                    bool isConcreteArrayReturn = actualClassName.find("[]") != std::string::npos;

                    if (!(isGenericArrayReturn && isConcreteArrayReturn))
                    {
                        // Use TypeValidator for detailed class compatibility checking
                        if (!actualClassName.empty() && expectedReturnType != "object")
                        {
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
        }
        else
        {
            // Function returns nothing (return;)
            // For async functions, allow return; in Promise<void> functions
            bool isAsyncVoidReturn = ctx.functionFrameManager.currentFrame().isAsync &&
                expectedReturnType == "Promise<void>";

            if (expectedReturnType != "void" && !isAsyncVoidReturn)
            {
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
        if (returnValueSlot == SIZE_MAX)
        {
            // First return in this try block - allocate a new slot
            returnValueSlot = ctx.variableTracker.getNextLocalSlot();
            ctx.functionFrameManager.updateMaxLocalSlot(returnValueSlot + 1);

            // Remember this slot so ControlFlowCompiler can load it back after finally
            ctx.exceptionManager.setReturnValueSlot(returnValueSlot);
        }

        if (returnValue)
        {
            // For async functions, wrap in Promise before storing
            // This ensures consistency - we always store a Promise for async functions
            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            // Store return value in the special slot
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(returnValueSlot), node);
        }
        else
        {
            // Push null for void return
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);

            // For async functions, wrap in Promise before storing for finally
            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
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
        if (outerReturnValueSlot == SIZE_MAX)
        {
            outerReturnValueSlot = ctx.variableTracker.getNextLocalSlot();
            ctx.functionFrameManager.updateMaxLocalSlot(outerReturnValueSlot + 1);
            ctx.exceptionManager.setReturnValueSlotForOuter(outerReturnValueSlot);
        }

        if (returnValue)
        {
            // Wrap in Promise if needed
            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }
            // Store return value in outer slot
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(outerReturnValueSlot),
                                         node);
        }
        else
        {
            // Push null for void return
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);

            // For async functions, wrap in Promise
            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            // Store return value
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint32_t>(outerReturnValueSlot),
                                         node);
        }

        // Jump to outer finally
        size_t returnJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
        ctx.exceptionManager.registerReturnJumpWithOuter(returnJump);
    }

    void FunctionCompiler::emitReturnValueBytecode(ast::ReturnNode* node, ast::ASTNode* returnValue)
    {
        // Use returnValue's location if available (fallback for when ReturnNode lacks proper location)
        ast::ASTNode* locationNode = (returnValue && returnValue->getLocation().getLine() > 0) ? returnValue : node;

        if (returnValue)
        {
            // Wrap in Promise if needed and return immediately
            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, locationNode);
            }
            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, locationNode);
        }
        else
        {
            // For async functions, treat return; as return null; wrapped in Promise
            // This allows Promise<void> functions to use return; like regular void functions
            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            }
            else
            {
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
        if (returnValue)
        {
            // PHASE 4: Try auto-boxing for return statements
            bool autoBoxed = tryEmitReturnAutoBoxing(returnValue);

            // If not auto-boxed, compile value normally
            if (!autoBoxed)
            {
                returnValue->accept(ctx.visitor);
            }
        }

        // Check if in function context
        if (ctx.functionFrameManager.isInFunction())
        {
            // Check if we're in a try block with a finally, but NOT already inside the finally block
            // If we're IN the finally block, we should return directly (not jump to finally again)
            if (ctx.exceptionManager.hasPendingFinally() && !ctx.exceptionManager.isInFinally())
            {
                emitReturnWithFinally(node, returnValue);
            }
            else if (ctx.exceptionManager.isInFinally() && ctx.exceptionManager.hasOuterFinally())
            {
                // We're in a finally block, but there's an outer finally that must execute
                // Store return value and jump to outer finally instead of returning immediately
                emitReturnWithOuterFinally(node, returnValue);
            }
            else
            {
                emitReturnValueBytecode(node, returnValue);
            }
        }
        else
        {
            // Not in a function context - fallback handling
            if (returnValue)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            }
            else
            {
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
        size_t currentFrameStart = ctx.functionFrameManager.isInFunction()
                                       ? ctx.functionFrameManager.currentFrame().localStartSlot
                                       : 0;

        for (const auto& local : currentLocals)
        {
            if (local.slot >= currentFrameStart)
            {
                capturedVars.push_back(local);
                // Mark this variable as captured to prevent slot reuse
                ctx.variableTracker.markVariableAsCaptured(local.slot);
            }
        }

        return capturedVars;
    }

    void FunctionCompiler::setupLambdaFrame(ast::LambdaNode* node,
                                            const std::vector<variables::VariableTracker::LocalVariable>& capturedVars)
    {
        const auto& params = node->getParameters();

        // Validate that lambda parameters don't shadow outer scope variables (C# behavior)
        for (const auto& param : params)
        {
            for (const auto& captured : capturedVars)
            {
                if (param.name == captured.name)
                {
                    throw errors::TypeException(
                        "Lambda parameter '" + param.name + "' shadows outer scope variable. "
                        "Variable shadowing is not allowed in lambdas (C# semantics).",
                        node->getLocation());
                }
            }
        }

        // Enter function frame for lambda
        ctx.functionFrameManager.enterFunctionFrame("auto",
                                                    0, // Lambda parameters start from slot 0
                                                    ctx.variableTracker.getCurrentScopeDepth(),
                                                    true, // Mark this frame as a lambda
                                                    node->getIsAsync()); // Mark if async lambda

        // Store captured variable names in the frame for shadowing validation
        auto& currentFrame = ctx.functionFrameManager.currentFrame();
        for (const auto& captured : capturedVars)
        {
            currentFrame.capturedVariableNames.push_back(captured.name);
        }

        ctx.variableTracker.beginScope();

        // Track lambda parameters as locals (they occupy slots 0, 1, 2, ...)
        for (const auto& param : params)
        {
            ctx.variableTracker.declareLocal(param.name, value::ValueType::VOID, "");
        }

        // Add captured variables as locals (they occupy slots after parameters)
        for (const auto& capture : capturedVars)
        {
            ctx.variableTracker.declareLocal(capture.name, capture.type, capture.className);
        }

        // Update max local slot
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
    }

    void FunctionCompiler::emitLambdaInstruction(size_t lambdaStart, ast::LambdaNode* node,
                                                 const std::vector<variables::VariableTracker::LocalVariable>&
                                                 capturedVars,
                                                 size_t currentFrameStart,
                                                 const std::vector<variables::VariableTracker::LocalVariable>&
                                                 currentLocals,
                                                 const std::string& lambdaFuncName)
    {
        const auto& params = node->getParameters();

        // Now emit instruction to create lambda value with captured environment
        // Operands layout: [lambdaStart, paramCount, captureCount, parentLocalCount, functionNameIdx,
        //                   captureSlot1, ..., captureSlotN,
        //                   paramNameIdx1, ..., paramNameIdxN,
        //                   capturedNameIdx1, ..., capturedNameIdxN,
        //                   parentNameIdx1, parentSlot1, ...]
        std::vector<uint32_t> operands;
        operands.push_back(static_cast<uint32_t>(lambdaStart));
        operands.push_back(static_cast<uint32_t>(params.size()));
        operands.push_back(static_cast<uint32_t>(capturedVars.size()));
        operands.push_back(static_cast<uint32_t>(currentLocals.size())); // Number of parent locals

        // Add lambda function name for metadata lookup
        size_t funcNameIdx = ctx.program.getConstantPool().addString(lambdaFuncName);
        operands.push_back(static_cast<uint32_t>(funcNameIdx));

        // Add captured variable slot numbers
        for (const auto& capture : capturedVars)
        {
            size_t relativeSlot = capture.slot - currentFrameStart;
            operands.push_back(static_cast<uint32_t>(relativeSlot));
        }

        // Add parameter names for debugging
        for (const auto& param : params)
        {
            size_t nameIndex = ctx.program.getConstantPool().addString(param.name);
            operands.push_back(static_cast<uint32_t>(nameIndex));
        }

        // Add captured variable names for debugging (in the same order as capture slots)
        for (const auto& capture : capturedVars)
        {
            size_t nameIndex = ctx.program.getConstantPool().addString(capture.name);
            operands.push_back(static_cast<uint32_t>(nameIndex));
        }

        // Add parent local variable name->slot mapping for late-bound access
        for (const auto& local : currentLocals)
        {
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
        size_t currentFrameStart = ctx.functionFrameManager.isInFunction()
                                       ? ctx.functionFrameManager.currentFrame().localStartSlot
                                       : 0;

        // Reset local slot counter for lambda's own scope
        ctx.variableTracker.resetLocalSlot();

        // Setup lambda frame with parameters and captured variables
        setupLambdaFrame(node, capturedVars);

        // IMPORTANT: Lambda body should compile in the same class context as the enclosing method
        // ctx.currentClassNode should already be set from the enclosing class/method compilation

        // Compile lambda body
        auto* body = node->getBody();
        if (node->isExpressionLambda())
        {
            // Expression lambda: () -> expr
            body->accept(ctx.visitor);

            // Wrap in Promise if async lambda
            if (node->getIsAsync())
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
        }
        else
        {
            // Block lambda: () -> { ... }
            // Create a new scope for the lambda body to allow shadowing of captured variables
            ctx.variableTracker.beginScope();

            body->accept(ctx.visitor);

            // End the lambda body scope
            ctx.variableTracker.endScope();

            // Implicit return null if no explicit return
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);

            // Wrap in Promise if async lambda
            if (node->getIsAsync())
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
        }

        // Capture local variable names for debugging (before exiting frame)
        const auto& locals = ctx.variableTracker.getLocals();
        const auto& currentFrame = ctx.functionFrameManager.currentFrame();
        size_t localCount = ctx.functionFrameManager.getLocalCount();
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
        metadata.returnType = "auto"; // Lambda return type is inferred
        metadata.isNative = false;
        metadata.isAsync = node->getIsAsync();
        metadata.localVariableNames = localVarNames;
        ctx.program.registerFunction(lambdaFuncName, metadata);

        // Emit lambda instruction with captured environment
        emitLambdaInstruction(lambdaStart, node, capturedVars, currentFrameStart, currentLocals, lambdaFuncName);

        // Restore previous nextLocalSlot
        ctx.variableTracker.setLocalSlot(savedNextLocalSlot);

        return std::monostate{};
    }

    bool FunctionCompiler::isValidTypeName(const std::string& typeName,
                                           const std::vector<std::string>& validGenericParams)
    {
        // Extract base type name first (handle generics like "List<T>", "Array<K>")
        std::string baseTypeName = typeName;
        size_t anglePos = typeName.find('<');
        if (anglePos != std::string::npos)
        {
            baseTypeName = typeName.substr(0, anglePos);
        }

        // Check if base type is a primitive type (including Array for array types, object for generic constraints, and Promise for async/await)
        if (baseTypeName == "int" || baseTypeName == "float" || baseTypeName == "string" ||
            baseTypeName == "bool" || baseTypeName == "void" || baseTypeName == "Array" ||
            baseTypeName == "object" || baseTypeName == "Promise")
        {
            return true;
        }

        // Check if it's a declared generic type parameter (check full type name, not base)
        for (const auto& genericParam : validGenericParams)
        {
            if (typeName == genericParam)
            {
                return true;
            }
        }

        // Check if base type is an existing class or interface
        if (ctx.environment->findClass(baseTypeName) != nullptr)
        {
            return true;
        }
        if (ctx.environment->findInterface(baseTypeName) != nullptr)
        {
            return true;
        }

        return false;
    }

    bool FunctionCompiler::tryEmitReturnAutoBoxing(ast::ASTNode* returnValue)
    {
        // Only try auto-boxing if we're in a function context
        if (!ctx.functionFrameManager.isInFunction() || !returnValue)
        {
            return false;
        }

        // Get expected return type from function frame
        std::string expectedReturnType = ctx.functionFrameManager.currentFrame().returnType;

        // Only auto-box for primitive Box types (Int, Float, Bool, String)
        bool isBoxType = (expectedReturnType == "Int" || expectedReturnType == "Float" ||
                          expectedReturnType == "Bool" || expectedReturnType == "String");
        if (!isBoxType)
        {
            return false;
        }

        // Check if returnValue is a primitive literal that needs boxing
        bool needsBoxing = false;
        ast::ASTNode* literalToBox = nullptr;

        if (expectedReturnType == "Int" && dynamic_cast<ast::IntegerNode*>(returnValue))
        {
            needsBoxing = true;
            literalToBox = returnValue;
        }
        else if (expectedReturnType == "Float" && dynamic_cast<ast::FloatNode*>(returnValue))
        {
            needsBoxing = true;
            literalToBox = returnValue;
        }
        else if (expectedReturnType == "Bool" && dynamic_cast<ast::BoolNode*>(returnValue))
        {
            needsBoxing = true;
            literalToBox = returnValue;
        }
        else if (expectedReturnType == "String" && dynamic_cast<ast::StringNode*>(returnValue))
        {
            needsBoxing = true;
            literalToBox = returnValue;
        }

        if (!needsBoxing)
        {
            return false;
        }

        // PHASE 4 AUTO-BOXING: Emit bytecode for boxing
        // Equivalent to: return new ExpectedType(literalValue);

        // 1. Compile the literal value (pushes it onto stack)
        literalToBox->accept(ctx.visitor);

        // 2. Emit NEW_OBJECT for the Box class
        size_t classNameIndex = ctx.program.getConstantPool().addString(expectedReturnType);
        ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                     static_cast<uint32_t>(classNameIndex),
                                     1u, // 1 constructor argument
                                     literalToBox);

        return true; // Auto-boxing was applied
    }
}
