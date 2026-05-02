#include "JitHelpers.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
#include "../../value/ValueTypeUtils.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/klass/FieldDefinition.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../runtime/context/ExecutionContext.hpp"

namespace vm::jit
{
    // Mirrors VariableExecutor::tryLoadFromInstanceField. Returns true if the
    // name resolved to a field on the current `this` and writes the field's
    // value into *dest.
    static bool jitTryLoadFromInstanceField(value::Value* dest,
                                            JitContext* ctx,
                                            const std::string& varName)
    {
        if (!ctx->vm) return false;
        const auto& callStack = ctx->vm->getCallStack();
        // MYT-208: accept stack-promoted `this`.
        if (callStack.empty() || !callStack.back().getThisInstanceRaw()) return false;

        auto* thisInstance = callStack.back().getThisInstanceRaw();
        if (!thisInstance->getField(varName)) return false;

        *dest = thisInstance->getFieldValue(varName);
        return true;
    }

    // Mirrors VariableExecutor::tryLoadFromStaticField. Parses the current
    // call frame's "ClassName::methodName" function name, looks up the class,
    // and resolves the static field.
    static bool jitTryLoadFromStaticField(value::Value* dest,
                                          JitContext* ctx,
                                          const std::string& varName)
    {
        if (!ctx->vm) return false;
        const auto& callStack = ctx->vm->getCallStack();
        if (callStack.empty()) return false;

        // MYT-197: prefer frame.definingClassName; fall back to resolving
        // the interned handle and splitting only for frames that lack it.
        std::string className;
        if (!callStack.back().definingClassName.empty()) {
            className = callStack.back().definingClassName;
        } else {
            const std::string& functionName = ctx->program->getFrameName(callStack.back().functionName);
            size_t colonPos = functionName.find("::");
            if (colonPos == std::string::npos) return false;
            className = functionName.substr(0, colonPos);
        }
        auto classRegistry = ctx->environment->getClassRegistry();
        if (!classRegistry) return false;

        auto classDef = classRegistry->findClass(className);
        if (!classDef) return false;

        const auto& staticFields = classDef->getStaticFields();
        auto it = staticFields.find(varName);
        if (it == staticFields.end()) return false;

        *dest = it->second->getValue();
        return true;
    }

    void jit_load_var(value::Value* dest, JitContext* ctx, uint32_t nameIndex)
    {
        if (ctx->pendingException) return;

        try
        {
            const std::string& varName =
                ctx->program->getConstantPool().getString(nameIndex);

            auto varDef = ctx->environment->findVariable(varName);
            if (varDef)
            {
                *dest = varDef->getValue();
                return;
            }

            if (jitTryLoadFromInstanceField(dest, ctx, varName)) return;
            if (jitTryLoadFromStaticField(dest, ctx, varName)) return;

            throw errors::RuntimeException("Variable not found: " + varName);
        }
        catch (const OSRDeoptException&)
        {
            // JIT-AWAIT deopt must unwind to the call boundary; never stash.
            throw;
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }

    // Mirrors VariableExecutor::tryStoreToInstanceField. Throws on final-field
    // violation; the catch block in jit_store_var routes the throw into
    // ctx->pendingException.
    static bool jitTryStoreToInstanceField(JitContext* ctx,
                                           const std::string& varName,
                                           const value::Value& val)
    {
        if (!ctx->vm) return false;
        const auto& callStack = ctx->vm->getCallStack();
        // MYT-208: accept stack-promoted `this`.
        if (callStack.empty() || !callStack.back().getThisInstanceRaw()) return false;

        auto* thisInstance = callStack.back().getThisInstanceRaw();
        auto fieldDef = thisInstance->getField(varName);
        if (!fieldDef) return false;

        // MYT-189: match VariableExecutor / ObjectExecutor — static finals
        // share FieldDefinition's isInitialized bit; instance finals track
        // per-instance via the fieldValues map.
        if (fieldDef->isFinal())
        {
            if (fieldDef->isStatic())
            {
                if (fieldDef->isInitialized())
                {
                    throw errors::RuntimeException(
                        "Cannot assign to final field '" + varName + "'");
                }
            }
            else
            {
                size_t idx = thisInstance->getClassDefinition()->getFieldIndex(varName);
                if (idx != SIZE_MAX)
                {
                    if (!value::isVoid(thisInstance->getFieldByIndex(idx)))
                    {
                        throw errors::RuntimeException(
                            "Cannot assign to final field '" + varName + "'");
                    }
                }
            }
        }

        thisInstance->setField(varName, val);
        return true;
    }

    static bool jitTryStoreToStaticField(JitContext* ctx,
                                         const std::string& varName,
                                         const value::Value& val)
    {
        if (!ctx->vm) return false;
        const auto& callStack = ctx->vm->getCallStack();
        if (callStack.empty()) return false;

        // MYT-197: prefer frame.definingClassName; fall back to resolving
        // the interned handle and splitting only when unset.
        std::string className;
        if (!callStack.back().definingClassName.empty()) {
            className = callStack.back().definingClassName;
        } else {
            const std::string& functionName = ctx->program->getFrameName(callStack.back().functionName);
            size_t colonPos = functionName.find("::");
            if (colonPos == std::string::npos) return false;
            className = functionName.substr(0, colonPos);
        }
        auto classRegistry = ctx->environment->getClassRegistry();
        if (!classRegistry) return false;

        auto classDef = classRegistry->findClass(className);
        if (!classDef) return false;

        const auto& staticFields = classDef->getStaticFields();
        auto it = staticFields.find(varName);
        if (it == staticFields.end()) return false;

        if (it->second->isFinal() && it->second->isInitialized())
        {
            throw errors::RuntimeException(
                "Cannot assign to final static field '" + varName + "'");
        }

        it->second->setValue(val);
        return true;
    }

    void jit_store_var(JitContext* ctx, uint32_t nameIndex,
                       const value::Value* val)
    {
        if (ctx->pendingException) return;

        try
        {
            const std::string& varName =
                ctx->program->getConstantPool().getString(nameIndex);

            auto varDef = ctx->environment->findVariable(varName);
            if (varDef)
            {
                if (varDef->isFinal())
                {
                    throw errors::RuntimeException(
                        "Cannot assign to final variable '" + varName + "'");
                }
                varDef->setValue(*val);
                return;
            }

            if (jitTryStoreToInstanceField(ctx, varName, *val)) return;
            if (jitTryStoreToStaticField(ctx, varName, *val)) return;

            throw errors::RuntimeException("Variable not found: " + varName);
        }
        catch (const OSRDeoptException&)
        {
            throw;
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }

    // MYT-208: mirrors VariableExecutor::handleDeclareVar. Registers a new
    // global variable in the environment with the given name, type inferred
    // from the value, and final flag. Used by the JIT emitter for DECLARE_VAR
    // (which only appears in JIT-compiled code as dead bytecode trailing a
    // function whose metadata range overcounts; the helper still has to be
    // semantically correct in case any reachable path ever invokes it).
    void jit_declare_var(JitContext* ctx, uint32_t nameIndex,
                         const value::Value* val, uint8_t isFinal)
    {
        if (ctx->pendingException) return;

        try
        {
            const std::string& varName =
                ctx->program->getConstantPool().getString(nameIndex);
            value::ValueType type = value::ValueTypeUtils::getValueType(*val);
            auto varDef = std::make_shared<runtimeTypes::global::VariableDefinition>(
                varName, type, *val, isFinal != 0);
            ctx->environment->declareVariable(varName, varDef);
        }
        catch (const OSRDeoptException&)
        {
            throw;
        }
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }
}
