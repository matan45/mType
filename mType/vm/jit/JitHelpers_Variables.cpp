#include "JitHelpers.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../../environment/Environment.hpp"
#include "../../environment/registry/ClassRegistry.hpp"
#include "../../runtimeTypes/global/VariableDefinition.hpp"
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
        if (callStack.empty() || !callStack.back().thisInstance) return false;

        auto thisInstance = callStack.back().thisInstance;
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

        const std::string& functionName = callStack.back().functionName;
        size_t colonPos = functionName.find("::");
        if (colonPos == std::string::npos) return false;

        std::string className = functionName.substr(0, colonPos);
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
        if (callStack.empty() || !callStack.back().thisInstance) return false;

        auto thisInstance = callStack.back().thisInstance;
        auto fieldDef = thisInstance->getField(varName);
        if (!fieldDef) return false;

        if (fieldDef->isFinal() && fieldDef->isInitialized())
        {
            throw errors::RuntimeException(
                "Cannot assign to final field '" + varName + "'");
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

        const std::string& functionName = callStack.back().functionName;
        size_t colonPos = functionName.find("::");
        if (colonPos == std::string::npos) return false;

        std::string className = functionName.substr(0, colonPos);
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
        catch (...)
        {
            ctx->pendingException = std::current_exception();
        }
    }
}
