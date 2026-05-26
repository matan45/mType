#include "FunctionCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../ast/nodes/functions/ReturnNode.hpp"

namespace vm::compiler::visitors
{
    namespace
    {
        bool unwrapAsyncPromiseReturn(const std::string& returnType, std::string& innerType)
        {
            if (returnType.find("Promise<") != 0)
            {
                return false;
            }

            size_t start = returnType.find('<') + 1;
            size_t end = returnType.rfind('>');
            if (start == std::string::npos || end == std::string::npos || end <= start)
            {
                return false;
            }

            innerType = returnType.substr(start, end - start);
            innerType.erase(0, innerType.find_first_not_of(" \t"));
            innerType.erase(innerType.find_last_not_of(" \t") + 1);
            return true;
        }

        value::ValueType declaredTypeToValueType(const std::string& typeName)
        {
            if (typeName == "int") return value::ValueType::INT;
            if (typeName == "float") return value::ValueType::FLOAT;
            if (typeName == "string") return value::ValueType::STRING;
            if (typeName == "bool") return value::ValueType::BOOL;
            if (typeName == "void") return value::ValueType::VOID;
            if (typeName.find("Array<") == 0 || typeName.find("[]") != std::string::npos) {
                return value::ValueType::ARRAY;
            }
            return value::ValueType::OBJECT;
        }

        bool isPrimitiveWrapperForValueType(const std::string& className, value::ValueType type)
        {
            return (className == "Int" && type == value::ValueType::INT) ||
                   (className == "Float" && type == value::ValueType::FLOAT) ||
                   (className == "Bool" && type == value::ValueType::BOOL) ||
                   (className == "String" && type == value::ValueType::STRING);
        }
    }

    // MYT-352: drain open stack-object scopes before a return transfers control.
    // Mirrors the break/continue precedent (ControlFlowCompiler::compileBreak /
    // compileContinue) so the LEAVE-before-jump invariant holds for *all* control-
    // flow exits, not just loop-exits. Without this, a callee inlined into a hot
    // loop would leak pool slots into the caller's frame on every iteration:
    // inlining elides the callee frame teardown that was the implicit backstop.
    // Safe by the escape-analysis invariant: anything in stackObjects[] at return
    // time has no live references past the innermost open scope.
    void FunctionCompiler::emitStackScopeLeavesBeforeReturn(ast::ASTNode* node)
    {
        const uint32_t depth = ctx.loopManager.getCurrentStackScopeDepth();
        for (uint32_t i = 0; i < depth; ++i)
        {
            ctx.emitter.emitWithLocation(bytecode::OpCode::STACK_SCOPE_LEAVE, node);
        }
    }

    void FunctionCompiler::validateReturnType(ast::ReturnNode* node, ast::ASTNode* returnValue)
    {
        if (!ctx.functionFrameManager.isInFunction())
        {
            return;
        }

        std::string expectedReturnType = ctx.functionFrameManager.currentFrame().returnType;

        if (expectedReturnType == "auto")
        {
            return;
        }

        bool returnTypeNullable = ::types::TypeConversionUtils::isNullableType(expectedReturnType);
        expectedReturnType = ::types::TypeConversionUtils::stripNullable(expectedReturnType);
        std::string validationReturnType = expectedReturnType;
        if (ctx.functionFrameManager.currentFrame().isAsync)
        {
            std::string promiseInnerType;
            if (unwrapAsyncPromiseReturn(expectedReturnType, promiseInnerType))
            {
                validationReturnType = promiseInnerType;
            }
        }

        if (returnValue)
        {
            // MYT-280: only skip the null-safety check when the return type is a
            // generic parameter that COULD be instantiated nullable (unbound, or
            // any constraint nullable). A T bound by a non-nullable type cannot
            // be null at any legal instantiation. The cheap syntactic gate stays
            // as a fast-path (skips the helper for any type name that isn't a
            // single uppercase letter).
            bool skipForGeneric =
                ::types::TypeConversionUtils::isGenericTypeParameter(expectedReturnType)
                && isGenericParamWithPossiblyNullableInstantiation(expectedReturnType);
            if (!returnTypeNullable && !skipForGeneric && ctx.typeInference.inferExpressionNullable(returnValue))
            {
                throw errors::TypeException(
                    "Cannot return nullable value from function with non-nullable return type '" +
                    expectedReturnType + "'. Use '" + expectedReturnType + "?' to allow null returns.",
                    node->getLocation()
                );
            }

            value::ValueType actualType = ctx.typeInference.inferExpressionType(returnValue);

            value::ValueType expectedType = declaredTypeToValueType(validationReturnType);

            if (actualType != value::ValueType::VOID && expectedType != value::ValueType::VOID)
            {
                if (actualType != expectedType)
                {
                    if (!(expectedType == value::ValueType::OBJECT && dynamic_cast<ast::NullNode*>(returnValue)))
                    {
                        if (!(expectedType == value::ValueType::FLOAT && actualType == value::ValueType::INT))
                        {
                            bool canAutoBox = false;
                            if (expectedType == value::ValueType::OBJECT)
                            {
                                if (isPrimitiveWrapperForValueType(validationReturnType, actualType))
                                {
                                    canAutoBox = true;
                                }

                                // MYT-281: arrays widen to Object on return, mirroring
                                // assignment / parameter passing.
                                if (expectedReturnType == "Object" &&
                                    actualType == value::ValueType::ARRAY)
                                {
                                    canAutoBox = true;
                                }
                            }

                            if (!canAutoBox && expectedType != value::ValueType::OBJECT &&
                                actualType == value::ValueType::OBJECT)
                            {
                                std::string actualClassName = ctx.typeInference.inferExpressionClassName(returnValue);
                                if (isPrimitiveWrapperForValueType(actualClassName, expectedType))
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
                else if (expectedType == value::ValueType::OBJECT && actualType == value::ValueType::OBJECT)
                {
                    std::string actualClassName = ctx.typeInference.inferExpressionClassName(returnValue);

                    bool isGenericArrayReturn = (expectedReturnType == "Array" || expectedReturnType.find("Array<") == 0);
                    bool isConcreteArrayReturn = actualClassName.find("[]") != std::string::npos;

                    if (!(isGenericArrayReturn && isConcreteArrayReturn))
                    {
                        if (!actualClassName.empty() && expectedReturnType != "object")
                        {
                            bool isNullValue = ctx.typeInference.isEffectivelyNullLiteral(returnValue);
                            ctx.typeValidator.validateAssignment(
                                expectedType, validationReturnType,
                                actualType, actualClassName,
                                isNullValue, node->getLocation()
                            );
                        }
                    }
                }
                // MYT-278: ARRAY-vs-ARRAY needs element-type comparison via class name,
                // mirroring the assignment path.
                else if (expectedType == value::ValueType::ARRAY && actualType == value::ValueType::ARRAY)
                {
                    std::string actualClassName = ctx.typeInference.inferExpressionClassName(returnValue);

                    bool isGenericArrayReturn = (expectedReturnType == "Array" || expectedReturnType.find("Array<") == 0);
                    bool isConcreteArrayReturn = actualClassName.find("[]") != std::string::npos;

                    if (!(isGenericArrayReturn && isConcreteArrayReturn) && !actualClassName.empty())
                    {
                        bool isNullValue = ctx.typeInference.isEffectivelyNullLiteral(returnValue);
                        ctx.typeValidator.validateAssignment(
                            expectedType, expectedReturnType,
                            actualType, actualClassName,
                            isNullValue, node->getLocation()
                        );
                    }
                }
            }
        }
        else
        {
            // Bare `return;`: allowed for void, async Promise<void>, and ctors (MYT-112).
            bool isAsyncVoidReturn = ctx.functionFrameManager.currentFrame().isAsync &&
                expectedReturnType == "Promise<void>";
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
        // Reuse the same return-value slot across all returns in the same try-finally
        // so the finally block always loads the correct value.
        size_t returnValueSlot = ctx.exceptionManager.getReturnValueSlot();
        if (returnValueSlot == SIZE_MAX)
        {
            returnValueSlot = ctx.variableTracker.getNextLocalSlot();
            ctx.functionFrameManager.updateMaxLocalSlot(returnValueSlot + 1);
            ctx.exceptionManager.setReturnValueSlot(returnValueSlot);
        }

        size_t relativeReturnSlot = returnValueSlot;
        if (ctx.functionFrameManager.isInFunction()) {
            size_t startSlot = ctx.functionFrameManager.currentFrame().localStartSlot;
            relativeReturnSlot = returnValueSlot - startSlot;
        }

        if (returnValue)
        {
            // Async: wrap in Promise before storing so we always store a Promise.
            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeReturnSlot), node);
        }
        else
        {
            // MYT-112: bare `return;` inside a constructor returns `this`, not null.
            if (ctx.functionFrameManager.currentFrame().isConstructor)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
            }
            else
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
            }

            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeReturnSlot), node);
        }

        emitStackScopeLeavesBeforeReturn(node);

        size_t returnJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
        ctx.exceptionManager.registerReturnJump(returnJump);
    }

    void FunctionCompiler::emitReturnWithOuterFinally(ast::ReturnNode* node, ast::ASTNode* returnValue)
    {
        size_t outerReturnValueSlot = ctx.exceptionManager.getReturnValueSlotForOuter();
        if (outerReturnValueSlot == SIZE_MAX)
        {
            outerReturnValueSlot = ctx.variableTracker.getNextLocalSlot();
            ctx.functionFrameManager.updateMaxLocalSlot(outerReturnValueSlot + 1);
            ctx.exceptionManager.setReturnValueSlotForOuter(outerReturnValueSlot);
        }

        size_t relativeOuterReturnSlot = outerReturnValueSlot;
        if (ctx.functionFrameManager.isInFunction()) {
            size_t startSlot = ctx.functionFrameManager.currentFrame().localStartSlot;
            relativeOuterReturnSlot = outerReturnValueSlot - startSlot;
        }

        if (returnValue)
        {
            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }
            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeOuterReturnSlot),
                                         node);
        }
        else
        {
            // MYT-112: bare `return;` inside a constructor returns `this`.
            if (ctx.functionFrameManager.currentFrame().isConstructor)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
            }
            else
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
            }

            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }

            ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL, static_cast<uint64_t>(relativeOuterReturnSlot),
                                         node);
        }

        emitStackScopeLeavesBeforeReturn(node);

        size_t returnJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
        ctx.exceptionManager.registerReturnJumpWithOuter(returnJump);
    }

    void FunctionCompiler::emitReturnValueBytecode(ast::ReturnNode* node, ast::ASTNode* returnValue)
    {
        // Fallback to ReturnNode when the value's location is missing/invalid.
        ast::ASTNode* locationNode = (returnValue && returnValue->getLocation().getLine() > 0) ? returnValue : node;

        if (returnValue)
        {
            if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, locationNode);
            }
            emitStackScopeLeavesBeforeReturn(locationNode);
            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, locationNode);
        }
        else
        {
            // MYT-112: bare `return;` in a constructor returns `this` (slot 0),
            // mirroring the implicit end-of-body return emitted by MethodCompilerHelper.
            if (ctx.functionFrameManager.currentFrame().isConstructor)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL, 0u, node);
                emitStackScopeLeavesBeforeReturn(node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            }
            // Async: treat return; as return null wrapped in a Promise so Promise<void>
            // functions can use return; like regular void functions.
            else if (ctx.functionFrameManager.currentFrame().isAsync)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
                emitStackScopeLeavesBeforeReturn(node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            }
            else
            {
                // Keep stack consistent — every call must leave exactly one value.
                ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
                emitStackScopeLeavesBeforeReturn(node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            }
        }
    }

    value::Value FunctionCompiler::compileReturn(ast::ReturnNode* node)
    {
        auto* returnValue = node->getReturnValue();

        validateReturnType(node, returnValue);

        if (returnValue)
        {
            bool autoBoxed = tryEmitReturnAutoBoxing(returnValue);

            if (!autoBoxed && !tryEmitReturnAutoUnboxing(returnValue))
            {
                returnValue->accept(ctx.visitor);
            }
        }

        if (ctx.functionFrameManager.isInFunction())
        {
            // If we're already inside the finally block, return directly — don't
            // jump to finally again.
            if (ctx.exceptionManager.hasPendingFinally() && !ctx.exceptionManager.isInFinally())
            {
                emitReturnWithFinally(node, returnValue);
            }
            else if (ctx.exceptionManager.isInFinally() && ctx.exceptionManager.hasOuterFinally())
            {
                emitReturnWithOuterFinally(node, returnValue);
            }
            else if (!ctx.exceptionManager.isInFinally() && ctx.exceptionManager.hasOuterFinally())
            {
                emitReturnWithOuterFinally(node, returnValue);
            }
            else
            {
                emitReturnValueBytecode(node, returnValue);
            }
        }
        else
        {
            // Not in a function context — fallback.
            if (returnValue)
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            }
            else
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
                ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
            }
        }

        return std::monostate{};
    }

    bool FunctionCompiler::tryEmitReturnAutoBoxing(ast::ASTNode* returnValue)
    {
        if (!ctx.functionFrameManager.isInFunction() || !returnValue)
        {
            return false;
        }

        std::string expectedReturnType = ctx.functionFrameManager.currentFrame().returnType;

        bool isBoxType = (expectedReturnType == "Int" || expectedReturnType == "Float" ||
                          expectedReturnType == "Bool" || expectedReturnType == "String");
        if (!isBoxType)
        {
            return false;
        }

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

        literalToBox->accept(ctx.visitor);

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

        return true;
    }

    bool FunctionCompiler::tryEmitReturnAutoUnboxing(ast::ASTNode* returnValue)
    {
        if (!ctx.functionFrameManager.isInFunction() || !returnValue)
        {
            return false;
        }

        std::string expectedReturnType = ctx.functionFrameManager.currentFrame().returnType;
        if (expectedReturnType == "auto")
        {
            return false;
        }

        expectedReturnType = ::types::TypeConversionUtils::stripNullable(expectedReturnType);
        std::string effectiveReturnType = expectedReturnType;
        if (ctx.functionFrameManager.currentFrame().isAsync)
        {
            std::string promiseInnerType;
            if (unwrapAsyncPromiseReturn(expectedReturnType, promiseInnerType))
            {
                effectiveReturnType = promiseInnerType;
            }
        }

        value::ValueType expectedType = declaredTypeToValueType(effectiveReturnType);
        if (expectedType != value::ValueType::INT &&
            expectedType != value::ValueType::FLOAT &&
            expectedType != value::ValueType::BOOL &&
            expectedType != value::ValueType::STRING)
        {
            return false;
        }

        if (ctx.typeInference.inferExpressionType(returnValue) != value::ValueType::OBJECT)
        {
            return false;
        }

        std::string actualClassName = ctx.typeInference.inferExpressionClassName(returnValue);
        if (!isPrimitiveWrapperForValueType(actualClassName, expectedType))
        {
            return false;
        }

        // Push the wrapper, then unbox via Box.getValue() — mirrors the assignment
        // path in StatementCompiler_Assignment.cpp:169-187 and the argument path
        // in FunctionCallHelper_Emit.cpp.
        returnValue->accept(ctx.visitor);
        size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
        ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                     static_cast<uint64_t>(methodNameIndex),
                                     0u,
                                     returnValue);
        return true;
    }
}
