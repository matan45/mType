#include "FunctionCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "FunctionCallHelper.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../ast/nodes/expressions/LambdaNode.hpp"
#include "../types/TypeNameValidator.hpp"

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

        // Native functions are implemented in C++ and already registered.
        const auto* existingFunc = ctx.program.getFunction(funcName);
        if (existingFunc && existingFunc->isNative)
        {
            return std::monostate{};
        }

        // MYT-218: track the enclosing function so visitors see method/function-
        // level generic type parameters; InstanceOf needs this to reject free
        // generic functions' T at compile time.
        ast::FunctionNode* wasFunctionNode = ctx.currentFunctionNode;
        ctx.currentFunctionNode = node;

        size_t skipJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
        size_t functionStart = ctx.program.getCurrentOffset();
        ctx.program.emit(bytecode::OpCode::PROFILE_ENTER);

        std::vector<std::string> validGenericParams;
        if (node->isGeneric())
        {
            for (const auto& param : node->getGenericTypeParameters())
            {
                validGenericParams.push_back(param.name);
            }
        }

        auto paramTypesVec = node->getParameterTypes();
        std::vector<std::string> paramNames;
        std::vector<std::string> paramTypes;
        for (const auto& param : paramTypesVec)
        {
            paramNames.push_back(param.first);
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

            if (!vm::compiler::types::isValidTypeName(paramTypeStr, validGenericParams, *ctx.env))
            {
                throw errors::TypeException(
                    "Undefined type '" + paramTypeStr + "' in parameter '" + param.first + "'. " +
                    "Type must be a primitive, declared generic parameter, or existing class/interface.",
                    node->getLocation()
                );
            }
        }

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

        if (!vm::compiler::types::isValidTypeName(returnTypeStr, validGenericParams, *ctx.env))
        {
            throw errors::TypeException(
                "Undefined type '" + returnTypeStr + "' in return type. " +
                "Type must be a primitive, declared generic parameter, or existing class/interface.",
                node->getLocation()
            );
        }

        ctx.functionFrameManager.enterFunctionFrame(funcName,
                                                    returnTypeStr,
                                                    ctx.variableTracker.getNextLocalSlot(),
                                                    ctx.variableTracker.getCurrentScopeDepth(),
                                                    false,
                                                    node->getIsAsync());
        ctx.variableTracker.beginScope();

        // Generic functions: push self-mapped bindings (T -> T) so body compiles
        // even before call-site resolution. MethodCompiler doesn't do this —
        // see project_method_compiler_no_self_mapping.md.
        bool pushedGenericBindings = false;
        if (node->isGeneric())
        {
            std::unordered_map<std::string, std::string> emptyBindings;
            const auto& genericParams = node->getGenericTypeParameters();
            for (const auto& param : genericParams)
            {
                emptyBindings[param.name] = param.name;
            }
            ctx.pushGenericTypeBindings(emptyBindings);
            pushedGenericBindings = true;
        }

        for (const auto& param : paramTypesVec)
        {
            ctx.variableTracker.declareLocal(param.first, param.second.basicType,
                                             param.second.className.value_or(""),
                                             param.second.nullable);
        }

        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());

        auto* body = node->getBodyPtr();
        if (body)
        {
            body->accept(ctx.visitor);
        }

        if (pushedGenericBindings)
        {
            ctx.popGenericTypeBindings();
        }

        // Implicit return for void / Promise<void> functions.
        bool isVoidFunction = (node->getReturnType() == value::ValueType::VOID);
        bool isAsyncVoidFunction = (node->getIsAsync() && returnTypeStr == "Promise<void>");

        if (isVoidFunction || isAsyncVoidFunction)
        {
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, node);
            if (node->getIsAsync())
            {
                ctx.emitter.emitWithLocation(bytecode::OpCode::CREATE_PROMISE, node);
            }
            ctx.emitter.emitWithLocation(bytecode::OpCode::RETURN_VALUE, node);
        }

        size_t localCount = ctx.functionFrameManager.getLocalCount();

        // Capture local variable names for debugging before exiting the frame.
        const auto& locals = ctx.variableTracker.getLocals();
        const auto& currentFrame = ctx.functionFrameManager.currentFrame();

        std::vector<std::string> localVarNames(localCount);
        for (const auto& local : locals)
        {
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

        size_t functionEnd = ctx.program.getCurrentOffset();
        ctx.emitter.patchJump(skipJump);

        // PHASE 2 FIX: preserve parameterTypeParameterUsage from the
        // FunctionRegistrar phase — required for ALL functions, not just
        // generics, because non-generic and generic overloads share the
        // unmangled name.
        const auto* existingMetadata = ctx.program.getFunction(funcName);

        bytecode::BytecodeProgram::FunctionMetadata metadata;
        metadata.name = funcName;
        metadata.startOffset = functionStart;
        metadata.instructionCount = functionEnd - functionStart;
        metadata.localCount = localCount;
        metadata.parameterCount = paramTypesVec.size();
        metadata.parameterNames = paramNames;
        metadata.parameterTypes = paramTypes;
        for (const auto& param : paramTypesVec)
        {
            metadata.parameterNullable.push_back(param.second.nullable);
        }
        metadata.returnType = returnTypeStr;
        metadata.isNative = false;
        metadata.isAsync = node->getIsAsync();
        metadata.localVariableNames = localVarNames;

        if (node->isGeneric())
        {
            const auto& genericParams = node->getGenericTypeParameters();
            for (const auto& param : genericParams)
            {
                metadata.genericTypeParameters.push_back(param.name);
            }
        }

        if (existingMetadata)
        {
            metadata.parameterTypeParameterUsage = existingMetadata->parameterTypeParameterUsage;
            metadata.exceptionTable = existingMetadata->exceptionTable;
        }

        std::string typeSignature = "";
        for (size_t i = 0; i < paramTypes.size(); ++i) {
            if (i > 0) typeSignature += ",";
            typeSignature += paramTypes[i];
        }
        std::string mangledName = typeSignature.empty() ? funcName : (funcName + "/" + typeSignature);

        metadata.mangledName = mangledName;
        ctx.program.registerFunction(funcName, metadata);
        ctx.program.registerFunction(mangledName, metadata);

        ctx.currentFunctionNode = wasFunctionNode;

        return std::monostate{};
    }

    value::Value FunctionCompiler::compileFunctionCall(ast::FunctionCallNode* node)
    {
        return callHelper->compileFunctionCall(node);
    }

    bool FunctionCompiler::isGenericParamWithPossiblyNullableInstantiation(
        const std::string& name) const
    {
        // Returns 1 (may be nullable) if name is a generic param in scope with
        // ANY nullable constraint or no constraints; 0 if all constraints are
        // non-nullable; -1 if not found in this scope.
        auto check = [&](const std::vector<ast::GenericTypeParameter>& params) -> int {
            for (const auto& p : params)
            {
                if (p.name != name) continue;
                if (p.constraints.empty()) return 1;
                for (const auto& c : p.constraints)
                {
                    if (::types::TypeConversionUtils::isNullableType(c)) return 1;
                }
                return 0;
            }
            return -1;
        };

        if (ctx.currentFunctionNode && ctx.currentFunctionNode->isGeneric())
        {
            int r = check(ctx.currentFunctionNode->getGenericTypeParameters());
            if (r != -1) return r == 1;
        }
        // project_method_compiler_no_self_mapping.md: MethodCompiler doesn't push
        // T->T, so walk method/class explicitly.
        if (ctx.currentMethodNode && ctx.currentMethodNode->isGeneric())
        {
            int r = check(ctx.currentMethodNode->getGenericTypeParameters());
            if (r != -1) return r == 1;
        }
        if (ctx.currentClassNode && ctx.currentClassNode->isGeneric())
        {
            int r = check(ctx.currentClassNode->getGenericParameters());
            if (r != -1) return r == 1;
        }
        return false;
    }
}
