#include "JitHelpers.hpp"
#include "JitHelpers_Objects.hpp"
#include "guards/DeoptimizationHandler.hpp"
#include "../../value/ValueShim.hpp"
#include "../../errors/RuntimeException.hpp"
#include "../runtime/utils/MethodResolver.hpp"
#include "../bytecode/BytecodeProgram.hpp"
#include "../runtime/VirtualMachine.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../../environment/registry/ClassDefinition.hpp"
#include "../../environment/registry/SignatureUtils.hpp"
#include "../../value/ValueObject.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace vm::jit
{
    bool trySpecializedCollectionCall(JitContext* ctx,
                                       const std::string& rawMethodName,
                                       size_t argCount)
    {
        if (!ctx || ctx->pendingException) return false;
        value::Value& receiverValue = ctx->callArgs[0];
        if (!value::isAnyObject(receiverValue)) return false;

        auto* receiver = value::asObjectInstanceRaw(receiverValue);
        if (!receiver) return false;

        auto* storage = receiver->getSpecializedCollection();
        if (!storage) return false;

        const std::string methodName =
            runtimeTypes::klass::SignatureUtils::extractSimpleName(rawMethodName);
        if (!storage->isSpecializedMethod(methodName, argCount)) return false;

        using Kind = value::SpecializedCollectionStorage::Kind;
        if (storage->getKind() == Kind::MAP)
        {
            if (methodName == "put")
            {
                value::Value oldValue;
                if (!storage->put(ctx->callArgs[1], ctx->callArgs[2], oldValue))
                    oldValue = nullptr;
                ctx->returnValue = oldValue;
                ctx->hasReturnValue = true;
                return true;
            }
            if (methodName == "get")
            {
                value::Value result;
                if (!storage->get(ctx->callArgs[1], result)) result = nullptr;
                ctx->returnValue = result;
                ctx->hasReturnValue = true;
                return true;
            }
            if (methodName == "containsKey")
            {
                ctx->returnValue = storage->containsKey(ctx->callArgs[1]);
                ctx->hasReturnValue = true;
                return true;
            }
            if (methodName == "remove")
            {
                ctx->returnValue = storage->remove(ctx->callArgs[1]);
                ctx->hasReturnValue = true;
                return true;
            }
            if (methodName == "getKeys" || methodName == "toArray")
            {
                ctx->returnValue = storage->materializeKeys(ctx->environment);
                ctx->hasReturnValue = true;
                return true;
            }
            if (methodName == "getValues")
            {
                ctx->returnValue = storage->materializeValues();
                ctx->hasReturnValue = true;
                return true;
            }
            if (methodName == "containsValue")
            {
                ctx->returnValue = storage->containsStoredValue(ctx->callArgs[1]);
                ctx->hasReturnValue = true;
                return true;
            }
        }
        else
        {
            if (methodName == "add")
            {
                ctx->returnValue = storage->add(ctx->callArgs[1]);
                ctx->hasReturnValue = true;
                return true;
            }
            if (methodName == "contains")
            {
                ctx->returnValue = storage->contains(ctx->callArgs[1]);
                ctx->hasReturnValue = true;
                return true;
            }
            if (methodName == "remove")
            {
                ctx->returnValue = storage->remove(ctx->callArgs[1]);
                ctx->hasReturnValue = true;
                return true;
            }
            if (methodName == "toArray")
            {
                ctx->returnValue = storage->materializeKeys(ctx->environment);
                ctx->hasReturnValue = true;
                return true;
            }
        }

        if (methodName == "size")
        {
            ctx->returnValue = static_cast<int64_t>(storage->size());
            ctx->hasReturnValue = true;
            return true;
        }
        if (methodName == "empty")
        {
            ctx->returnValue = storage->empty();
            ctx->hasReturnValue = true;
            return true;
        }
        if (methodName == "clear")
        {
            storage->clear();
            ctx->returnValue = std::monostate{};
            ctx->hasReturnValue = true;
            return true;
        }

        return false;
    }

    void jit_call_method(JitContext* ctx, uint32_t methodNameIndex, size_t argCount)
    {
        if (ctx->pendingException)
            return;

        try
        {
            value::Value& objectValue = ctx->callArgs[0];

            if (value::isLambda(objectValue))
            {
                if (ctx->vm)
                {
                    auto lambda = value::asLambda(objectValue);
                    ctx->returnValue = ctx->vm->invokeLambda(
                        lambda, &ctx->callArgs[1], argCount);
                    ctx->hasReturnValue = true;
                    return;
                }
            }

            const std::string& methodName = ctx->program->getConstantPool().getString(methodNameIndex);

            if (trySpecializedCollectionCall(ctx, methodName, argCount))
            {
                return;
            }

            std::vector<value::Value> args;
            for (size_t i = 1; i <= argCount; i++)
            {
                args.push_back(ctx->callArgs[i]);
            }

            // STACK_OBJECT receivers go through the Value-receiver overload so VM
            // dispatch tag-branches frame.thisInstance vs thisInstanceRaw correctly.
            // Heap (OBJECT) receivers stay on the shared_ptr fast path.
            if (value::isStackObject(objectValue))
            {
                if (ctx->vm)
                {
                    ctx->returnValue = ctx->vm->callMethodFromJit(objectValue, methodName, args);
                    ctx->hasReturnValue = true;
                    return;
                }
            }

            if (value::isObject(objectValue))
            {
                auto instance = value::asObject(objectValue);

                if (ctx->vm)
                {
                    ctx->returnValue = ctx->vm->callMethodFromJit(instance, methodName, args);
                    ctx->hasReturnValue = true;
                    return;
                }
            }

            if (value::isValueObject(objectValue))
            {
                if (ctx->vm)
                {
                    // Delegate to the Value-receiver VM overload, which
                    // batch-materialises the temp ObjectInstance via
                    // ObjectInstance::loadFromValueObject (vector copy + one
                    // hashmap fill) instead of the N × setField hot loop.
                    // In-method mutation semantics preserved — the temp is
                    // independent of the caller's ValueObject.
                    ctx->returnValue = ctx->vm->callMethodFromJit(objectValue, methodName, args);
                    ctx->hasReturnValue = true;
                    return;
                }
            }

            // LAMBDA receiver. Mirrors ObjectExecutor::handleCallMethod's
            // invokeLambdaMethod route. Without this branch, an inlined
            // lambda receiver routes to the throw below, the exception lands
            // in ctx->pendingException, and every subsequent jit_call_method_ic
            // becomes a silent no-op — turning the OSR'd outer loop into an
            // infinite spin (stream_pipeline_hot regression).
            if (value::isLambda(objectValue))
            {
                if (ctx->vm)
                {
                    auto lambda = value::asLambda(objectValue);
                    ctx->returnValue = ctx->vm->invokeLambda(
                        lambda, &ctx->callArgs[1], argCount);
                    ctx->hasReturnValue = true;
                    return;
                }
            }

            throw errors::RuntimeException("JIT: cannot call method '" + methodName + "'");
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

    // Function-side direct JIT-to-JIT dispatch (counterpart to
    // jit_call_method_direct). Caller pre-resolves frame name + callee
    // programIndex via the IC cold path; the warm-path call site enforces
    // the size gate that re-lands what MYT-321 reverted without re-introducing
    // the generic_dispatch_hot.mt regression.
    void jit_call_function_direct(JitContext* ctx, const JitDirectCallArgs& args)
    {
        if (ctx->pendingException) return;
        if (!args.cachedJit || !ctx->vm) return;
        const auto* funcMeta =
            static_cast<const bytecode::BytecodeProgram::FunctionMetadata*>(args.funcMetadata);
        if (!args.calleeProgram || !funcMeta) return;

        if (ctx->vm->getJitNativeDepth() >= vm::runtime::VirtualMachine::MAX_JIT_NATIVE_DEPTH)
        {
            try {
                throw errors::RuntimeException(
                    "JIT: native recursion depth exceeded in direct function dispatch");
            } catch (...) {
                ctx->pendingException = std::current_exception();
            }
            return;
        }

        auto fn = reinterpret_cast<void(*)(JitContext*)>(const_cast<void*>(args.cachedJit));

        const size_t savedStackSize = ctx->stackManager ? ctx->stackManager->size() : 0;
        vm::runtime::CallFrame frame;
        frame.returnAddress = ctx->vm->getInstructionPointer();
        frame.frameBase = savedStackSize;
        frame.localBase = savedStackSize;
        frame.functionName = args.frameName;
        frame.programIndex = args.calleeProgramIndex;
        frame.thisInstance = nullptr;
        ctx->vm->pushCallFrame(std::move(frame));

        JitContext nestedCtx{};
        nestedCtx.args = ctx->callArgs;
        nestedCtx.argCount = args.argCount;
        nestedCtx.hasReturnValue = false;
        nestedCtx.program = args.calleeProgram;
        nestedCtx.stackManager = ctx->stackManager;
        nestedCtx.environment = ctx->environment;
        nestedCtx.vm = ctx->vm;
        nestedCtx.jitCodeCache = ctx->jitCodeCache;
        nestedCtx.icTable = ctx->icTable;
        nestedCtx.callingClassName = ctx->callingClassName;

        ctx->vm->incrementJitNativeDepth();
        try {
            fn(&nestedCtx);
        } catch (...) {
            ctx->pendingException = std::current_exception();
        }
        ctx->vm->decrementJitNativeDepth();
        ctx->vm->popCallStack();

        if (nestedCtx.pendingException && !ctx->pendingException) {
            ctx->pendingException = nestedCtx.pendingException;
            return;
        }
        if (ctx->pendingException) return;

        ctx->returnValue = std::move(nestedCtx.returnValue);
        ctx->hasReturnValue = nestedCtx.hasReturnValue;
    }
}
