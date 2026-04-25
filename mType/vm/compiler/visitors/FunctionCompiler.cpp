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

        // Emit profiling opcode for JIT compilation support
        ctx.program.emit(bytecode::OpCode::PROFILE_ENTER);

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
        ctx.functionFrameManager.enterFunctionFrame(funcName,
                                                    returnTypeStr,
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

        // Track parameters as locals (preserving nullable status)
        for (const auto& param : paramTypesVec)
        {
            ctx.variableTracker.declareLocal(param.first, param.second.basicType,
                                             param.second.className.value_or(""),
                                             param.second.nullable);
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
        // Build per-parameter nullable flags
        for (const auto& param : paramTypesVec)
        {
            metadata.parameterNullable.push_back(param.second.nullable);
        }
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
        }

        // PHASE 2 FIX: Preserve parameterTypeParameterUsage from initial registration
        // This must be done for ALL functions, not just generic ones, because multiple overloads
        // (both generic and non-generic) share the same base name and we must not lose the data
        // when a non-generic overload is compiled
        if (existingMetadata)
        {
            metadata.parameterTypeParameterUsage = existingMetadata->parameterTypeParameterUsage;

            // Preserve exception table from existing metadata (built during body compilation)
            metadata.exceptionTable = existingMetadata->exceptionTable;
        }

        // Build mangled name for overload support
        std::string typeSignature = "";
        for (size_t i = 0; i < paramTypes.size(); ++i) {
            if (i > 0) typeSignature += ",";
            typeSignature += paramTypes[i];
        }
        std::string mangledName = typeSignature.empty() ? funcName : (funcName + "/" + typeSignature);

        // Register with BOTH original name and mangled name
        metadata.mangledName = mangledName;
        ctx.program.registerFunction(funcName, metadata);      // Original name
        ctx.program.registerFunction(mangledName, metadata);   // Mangled name

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

        // Determine if return type is nullable, then strip suffix for type matching
        bool returnTypeNullable = ::types::TypeConversionUtils::isNullableType(expectedReturnType);
        expectedReturnType = ::types::TypeConversionUtils::stripNullable(expectedReturnType);

        if (returnValue)
        {
            // Null safety enforcement: reject nullable returns from non-nullable return types
            // Skip for generic type parameters (T, K, V, etc.) since they may be nullable at instantiation
            bool isGenericReturnType = ::types::TypeConversionUtils::isGenericTypeParameter(expectedReturnType);
            if (!returnTypeNullable && !isGenericReturnType && ctx.typeInference.inferExpressionNullable(returnValue))
            {
                throw errors::TypeException(
                    "Cannot return nullable value from function with non-nullable return type '" +
                    expectedReturnType + "'. Use '" + expectedReturnType + "?' to allow null returns.",
                    node->getLocation()
                );
            }

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
                    // Special case: null can be returned for object types (nullable check done above)
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

            // MYT-112: constructors implicitly return `this`; bare `return;`
            // is a legitimate early-exit that emits the same LOAD_LOCAL 0
            // + RETURN_VALUE sequence as the end-of-body return.
            bool isConstructor = ctx.functionFrameManager.currentFrame().isConstructor;

            if (expectedReturnType != "void" && !isAsyncVoidReturn && !isConstructor)
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

        // Convert absolute slot to relative slot for STORE_LOCAL emission
        size_t relativeReturnSlot = returnValueSlot;
        if (ctx.functionFrameManager.isInFunction()) {
            size_t startSlot = ctx.functionFrameManager.currentFrame().localStartSlot;
            relativeReturnSlot = returnValueSlot - startSlot;
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
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeReturnSlot), node);
        }
        else
        {
            // MYT-112: bare `return;` inside a constructor returns `this`
            // (slot 0), not null.
            if (ctx.functionFrameManager.currentFrame().isConstructor)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
            }
            else
            {
                // Push null for void return
                ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
            }

            // For async functions, wrap in Promise before storing for finally
            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            // Store return value
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeReturnSlot), node);
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

        // Convert absolute slot to relative slot for STORE_LOCAL emission
        size_t relativeOuterReturnSlot = outerReturnValueSlot;
        if (ctx.functionFrameManager.isInFunction()) {
            size_t startSlot = ctx.functionFrameManager.currentFrame().localStartSlot;
            relativeOuterReturnSlot = outerReturnValueSlot - startSlot;
        }

        if (returnValue)
        {
            // Wrap in Promise if needed
            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }
            // Store return value in outer slot
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeOuterReturnSlot),
                                         node);
        }
        else
        {
            // MYT-112: bare `return;` inside a constructor returns `this`
            // (slot 0), not null.
            if (ctx.functionFrameManager.currentFrame().isConstructor)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
            }
            else
            {
                // Push null for void return
                ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
            }

            // For async functions, wrap in Promise
            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            // Store return value
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeOuterReturnSlot),
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
            // MYT-112: bare `return;` inside a constructor returns the
            // instance (local slot 0 = `this`), mirroring the implicit
            // end-of-body return emitted by MethodCompilerHelper.
            if (ctx.functionFrameManager.currentFrame().isConstructor)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            }
            // For async functions, treat return; as return null; wrapped in Promise
            // This allows Promise<void> functions to use return; like regular void functions
            else if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            }
            else
            {
                // Push null for void return to keep stack consistent —
                // all calls leave exactly one value on the stack
                ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
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
            else if (!ctx.exceptionManager.isInFinally() && ctx.exceptionManager.hasOuterFinally())
            {
                // We're in a try-catch (without finally), but there's an outer finally that must execute
                // Store return value and jump to outer finally
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
                // Push null for void return to keep stack consistent
                ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            }
        }

        return std::monostate{};
    }

    std::vector<variables::VariableTracker::LocalVariable> FunctionCompiler::captureScopeVariables()
    {
        std::vector<variables::VariableTracker::LocalVariable> capturedVars;
        const auto& currentLocals = ctx.variableTracker.getLocals();

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

    std::vector<std::string> FunctionCompiler::setupLambdaFrame(ast::LambdaNode* node,
                                            const std::vector<variables::VariableTracker::LocalVariable>& capturedVars,
                                            const std::string& lambdaFuncName)
    {
        const auto& params = node->getParameters();
        std::vector<std::string> parameterTypeNames; // Will store parameter type names for metadata

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
        ctx.functionFrameManager.enterFunctionFrame(lambdaFuncName,
                                                    "auto",
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

        // Resolve lambda parameter types from expected type context
        std::vector<std::pair<value::ValueType, std::string>> resolvedParamTypes;

        if (ctx.hasExpectedTypeContext())
        {
            auto expectedCtx = ctx.getCurrentExpectedTypeContext();

            if (expectedCtx.expectedType == value::ValueType::OBJECT)
            {
                std::string baseClassName = expectedCtx.getBaseClassName();

                // Check if this is an interface type
                auto interfaceDef = ctx.env->findInterface(baseClassName);

                if (interfaceDef && interfaceDef->isFunctionalInterface())
                {
                    auto* samMethod = interfaceDef->getFunctionalMethod();

                    if (samMethod) {
                    }

                    if (samMethod && samMethod->parameters.size() == params.size())
                    {
                        // Build generic bindings if interface is generic
                        std::unordered_map<std::string, std::string> bindings;

                        if (expectedCtx.hasGenericArguments())
                        {
                            auto genericArgs = expectedCtx.extractGenericArguments();
                            const auto& interfaceGenericParams = interfaceDef->getGenericParameters();

                            for (size_t i = 0; i < interfaceGenericParams.size() && i < genericArgs.size(); ++i)
                            {
                                bindings[interfaceGenericParams[i].name] = genericArgs[i];
                            }
                        }

                        // Resolve parameter types using bindings (or use concrete types directly)
                        for (size_t i = 0; i < samMethod->parameters.size(); ++i)
                        {
                            std::string paramTypeName = samMethod->parameters[i].second->toString();

                            // Resolve generic type parameters (T -> Int, etc.)
                            if (!bindings.empty())
                            {
                                auto it = bindings.find(paramTypeName);
                                if (it != bindings.end())
                                {
                                    paramTypeName = it->second;
                                }
                            }

                            // Determine ValueType and className from the resolved type name
                            value::ValueType paramType;
                            std::string paramClassName = "";

                            // Map type names to ValueType
                            if (paramTypeName == "int")
                            {
                                paramType = value::ValueType::INT;
                            }
                            else if (paramTypeName == "float")
                            {
                                paramType = value::ValueType::FLOAT;
                            }
                            else if (paramTypeName == "bool")
                            {
                                paramType = value::ValueType::BOOL;
                            }
                            else if (paramTypeName == "string")
                            {
                                paramType = value::ValueType::STRING;
                            }
                            else if (paramTypeName == "void")
                            {
                                paramType = value::ValueType::VOID;
                            }
                            else
                            {
                                // Object type (Int, Float, Bool, String, or custom classes)
                                paramType = value::ValueType::OBJECT;
                                paramClassName = paramTypeName;
                            }

                            resolvedParamTypes.push_back({paramType, paramClassName});
                        }
                    }
                }
            }
        }

        // Track lambda parameters as locals (they occupy slots 0, 1, 2, ...)
        for (size_t i = 0; i < params.size(); ++i)
        {
            if (i < resolvedParamTypes.size())
            {
                ctx.variableTracker.declareLocal(params[i].name, resolvedParamTypes[i].first, resolvedParamTypes[i].second);

                // Store parameter type name for metadata
                if (!resolvedParamTypes[i].second.empty()) {
                    // Object type with className
                    parameterTypeNames.push_back(resolvedParamTypes[i].second);
                } else {
                    // Primitive type - convert ValueType to string
                    std::string typeName;
                    switch (resolvedParamTypes[i].first) {
                        case value::ValueType::INT: typeName = "int"; break;
                        case value::ValueType::FLOAT: typeName = "float"; break;
                        case value::ValueType::BOOL: typeName = "bool"; break;
                        case value::ValueType::STRING: typeName = "string"; break;
                        case value::ValueType::VOID: typeName = "void"; break;
                        default: typeName = "object"; break;
                    }
                    parameterTypeNames.push_back(typeName);
                }
            }
            else
            {
                ctx.variableTracker.declareLocal(params[i].name, value::ValueType::VOID, "");
                parameterTypeNames.push_back("auto"); // Unknown type
            }
        }

        // Add captured variables as locals (they occupy slots after parameters)
        for (const auto& capture : capturedVars)
        {
            ctx.variableTracker.declareLocal(capture.name, capture.type, capture.className, capture.isNullable);
        }

        // Update max local slot
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        return parameterTypeNames;
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
        std::vector<uint64_t> operands;
        operands.push_back(static_cast<uint64_t>(lambdaStart));
        operands.push_back(static_cast<uint64_t>(params.size()));
        operands.push_back(static_cast<uint64_t>(capturedVars.size()));
        operands.push_back(static_cast<uint64_t>(currentLocals.size())); // Number of parent locals

        // Add lambda function name for metadata lookup
        size_t funcNameIdx = ctx.program.getConstantPool().addString(lambdaFuncName);
        operands.push_back(static_cast<uint64_t>(funcNameIdx));

        // Add captured variable slot numbers
        for (const auto& capture : capturedVars)
        {
            size_t relativeSlot = capture.slot - currentFrameStart;
            operands.push_back(static_cast<uint64_t>(relativeSlot));
        }

        // Add parameter names for debugging
        for (const auto& param : params)
        {
            size_t nameIndex = ctx.program.getConstantPool().addString(param.name);
            operands.push_back(static_cast<uint64_t>(nameIndex));
        }

        // Add captured variable names for debugging (in the same order as capture slots)
        for (const auto& capture : capturedVars)
        {
            size_t nameIndex = ctx.program.getConstantPool().addString(capture.name);
            operands.push_back(static_cast<uint64_t>(nameIndex));
        }

        // Add parent local variable name->slot mapping for late-bound access
        for (const auto& local : currentLocals)
        {
            size_t nameIndex = ctx.program.getConstantPool().addString(local.name);
            operands.push_back(static_cast<uint64_t>(nameIndex));
            operands.push_back(static_cast<uint64_t>(local.slot));
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

        // Pre-register lambda metadata so exception tables can be added during body compilation
        bytecode::BytecodeProgram::FunctionMetadata tempMetadata;
        tempMetadata.name = lambdaFuncName;
        tempMetadata.startOffset = lambdaStart;
        tempMetadata.instructionCount = 0;  // Will be updated after compilation
        tempMetadata.localCount = 0;        // Will be updated after compilation
        tempMetadata.parameterCount = node->getParameters().size();
        tempMetadata.returnType = "auto";
        tempMetadata.isAsync = node->getIsAsync();
        tempMetadata.isStatic = false;
        tempMetadata.isNative = false;
        ctx.program.registerFunction(lambdaFuncName, tempMetadata);

        // Setup lambda frame with parameters and captured variables
        std::vector<std::string> parameterTypes = setupLambdaFrame(node, capturedVars, lambdaFuncName);

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
        ctx.program.patchJump(skipJump, static_cast<uint64_t>(lambdaEnd));

        // Update lambda metadata (preserving exception table from body compilation)
        auto* existingMetadata = const_cast<bytecode::BytecodeProgram::FunctionMetadata*>(
            ctx.program.getFunction(lambdaFuncName)
        );

        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = lambdaFuncName;
        metadata.startOffset = lambdaStart;
        metadata.instructionCount = lambdaEnd - lambdaStart;
        metadata.localCount = localCount;
        metadata.parameterCount = node->getParameters().size();
        metadata.parameterTypes = parameterTypes; // Store parameter types for runtime auto-boxing
        metadata.returnType = "auto"; // Lambda return type is inferred
        metadata.isNative = false;
        metadata.isAsync = node->getIsAsync();
        metadata.localVariableNames = localVarNames;

        // Preserve exception table built during body compilation
        if (existingMetadata) {
            metadata.exceptionTable = existingMetadata->exceptionTable;
        }

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
        std::string baseTypeName = typeName;

        // Strip nullable suffix '?'
        baseTypeName = ::types::TypeConversionUtils::stripNullable(baseTypeName);

        // Handle array types: int[], T[], Item[][], etc.
        // Strip all array brackets to get the element type
        size_t bracketPos = baseTypeName.find('[');
        if (bracketPos != std::string::npos)
        {
            baseTypeName = baseTypeName.substr(0, bracketPos);
        }

        // Extract base type name (handle generics like "List<T>", "Array<K>")
        size_t anglePos = baseTypeName.find('<');
        if (anglePos != std::string::npos)
        {
            baseTypeName = baseTypeName.substr(0, anglePos);
        }

        // Check if base type is a primitive type (including Array for array types, object for generic constraints, and Promise for async/await)
        if (baseTypeName == "int" || baseTypeName == "float" || baseTypeName == "string" ||
            baseTypeName == "bool" || baseTypeName == "void" || baseTypeName == "Array" ||
            baseTypeName == "object" || baseTypeName == "Promise")
        {
            return true;
        }

        // Check if it's a declared generic type parameter (check element type for arrays)
        for (const auto& genericParam : validGenericParams)
        {
            if (baseTypeName == genericParam)
            {
                return true;
            }
        }

        // Check if base type is an existing class or interface
        if (ctx.env->findClass(baseTypeName) != nullptr)
        {
            return true;
        }
        if (ctx.env->findInterface(baseTypeName) != nullptr)
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

        // 2. Emit NEW_OBJECT or NEW_VALUE_OBJECT for the Box class
        size_t classNameIndex = ctx.program.getConstantPool().addString(expectedReturnType);
        auto boxClassDef = ctx.env->findClass(expectedReturnType);
        bool boxIsValue = boxClassDef && boxClassDef->isValueClass();
        if (boxIsValue) {
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_VALUE_OBJECT,
                                         static_cast<uint64_t>(classNameIndex),
                                         1u, literalToBox);
            ctx.emitter.emitWithLocation(bytecode::OpCode::OBJECT_TO_VALUE, literalToBox);
        } else {
            ctx.emitter.emitWithLocation(bytecode::OpCode::NEW_OBJECT,
                                         static_cast<uint64_t>(classNameIndex),
                                         1u, literalToBox);
        }

        return true; // Auto-boxing was applied
    }
}

