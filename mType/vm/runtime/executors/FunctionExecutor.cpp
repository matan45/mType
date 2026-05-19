#include "FunctionExecutor.hpp"
#include <cstddef>
#include "../../../constants/LambdaConstants.hpp"
#include "../../../debugger/DebugHookHelper.hpp"
#include "../../../environment/NativeContext.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"
#include "../../../environment/registry/InterfaceDefinition.hpp"
#include "../../../errors/SourceLocation.hpp"
#include "../../../value/NativeArray.hpp"
#include "../../../value/ObjectInstance.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include "../../../value/SmallArgsBuffer.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../jit/JitCodeCache.hpp"
#include "../../jit/JitContext.hpp"
#include "../VirtualMachine.hpp"
#include "../../profiler/ProfilerHookHelper.hpp"

namespace vm::runtime
{
    FunctionExecutor::FunctionExecutor(ExecutionContext& ctx,
                                       std::shared_ptr<environment::Environment> env,
                                       VirtualMachine* vmPtr)
        : context(ctx)
        , environment(std::move(env))
        , vm(vmPtr)
    {
    }

    void FunctionExecutor::handleCall(const bytecode::BytecodeProgram::Instruction& instr)
    {
        std::string functionName = context.program->getConstantPool().getString(instr.inlineOperands[0]);
        size_t argCount = instr.inlineOperands[1];

        // frameBase must be captured BEFORE popping arguments.
        size_t frameBase = context.stackManager->size() - argCount;

        // MYT-196: small-buffer-optimized scratch buffer for popped args.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i)
        {
            args[i - 1] = context.stackManager->pop();
        }

        // MYT-201: CALL IC state lives in the per-IP side table. The first
        // dispatch at this IP has no entry; subsequent ones hit the cache.
        const bytecode::BytecodeProgram::FunctionMetadata* funcMetadata = nullptr;
        size_t targetProgramIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;
        const bytecode::BytecodeProgram* targetProgram = context.program;

        const size_t callIp = context.instructionPointer;
        if (auto* cached = context.findCachedState(callIp))
        {
            if (cached->cachedFuncMetadata)
            {
                funcMetadata = cached->cachedFuncMetadata;
                targetProgram = cached->cachedProgram;
                targetProgramIndex = cached->cachedProgramIndex;
            }
            else if (cached->cachedNativeFunc)
            {
                // FFI cache hit — skip both unordered_map::find calls into the
                // native registry. The slot is populated below on first dispatch.
                vm::profiler::ProfilerHookHelper::onFunctionEntry(functionName);
                environment::NativeContext nativeCtx{ environment, vm->shared_from_this() };
                value::Value result = cached->cachedNativeFunc(nativeCtx, args.span());
                vm::profiler::ProfilerHookHelper::onFunctionExit(functionName);
                context.stackManager->push(result);
                return;
            }
        }

        if (!funcMetadata) {
            auto nativeRegistry = environment->getNativeRegistry();
            if (nativeRegistry && nativeRegistry->hasNativeFunction(functionName))
            {
                auto nativeFunc = nativeRegistry->findNativeFunction(functionName);
                if (nativeFunc)
                {
                    vm::profiler::ProfilerHookHelper::onFunctionEntry(functionName);

                    auto& newCached = context.getOrCreateCachedState(callIp);
                    newCached.cachedNativeFunc = nativeFunc;

                    environment::NativeContext nativeCtx{ environment, vm->shared_from_this() };
                    value::Value result = nativeFunc(nativeCtx, args.span());

                    vm::profiler::ProfilerHookHelper::onFunctionExit(functionName);

                    context.stackManager->push(result);
                    return;
                }
            }

            funcMetadata = context.program->getFunction(functionName);

            if (!funcMetadata) {
                const auto& loaded = vm->getLoadedPrograms();
                for (size_t i = 0; i < loaded.size(); ++i) {
                    auto libFunc = loaded[i]->getFunction(functionName);
                    if (libFunc) {
                        funcMetadata = libFunc;
                        targetProgramIndex = i;
                        targetProgram = loaded[i];
                        break;
                    }
                }
            }

            // Allocate the IC side-table entry only on first successful resolve —
            // keeps the generic path allocation-free for native-only and
            // missing-function sites.
            if (funcMetadata) {
                auto& cached = context.getOrCreateCachedState(callIp);
                cached.cachedFuncMetadata = funcMetadata;
                cached.cachedStartOffset = funcMetadata->startOffset;
                cached.cachedProgram = targetProgram;
                cached.cachedProgramIndex = targetProgramIndex;
            }
        }

        if (funcMetadata)
        {
            // Skip auto-box/unbox for primitive-only parameter lists.
            if (!funcMetadata->allPrimitiveParams)
                convertLambdaArgumentsToInterfaces(args.span(), funcMetadata->parameterTypes);

            CallFrame frame;
            frame.returnAddress = context.instructionPointer;
            frame.frameBase = frameBase;
            frame.localBase = context.stackManager->size();
            // MYT-197: intern on the target program so the handle belongs to
            // whatever program owns the function (cross-library calls).
            frame.functionName = targetProgram->internFrameName(functionName);
            frame.thisInstance = nullptr;
            frame.programIndex = targetProgramIndex;

            if (targetProgram != context.program) {
                context.program = targetProgram;
            }

            context.pushCallFrame(std::move(frame));
            context.stats.functionCalls++;

            vm::profiler::ProfilerHookHelper::onFunctionEntry(functionName);

            if (debugger::DebugHookHelper::isDebuggingEnabled())
            {
                auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
                if (sourceLoc)
                {
                    errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(functionName, errorsLoc);
                }
                else
                {
                    auto funcStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                    if (funcStartLoc)
                    {
                        errors::SourceLocation errorsLoc(funcStartLoc->filename, funcStartLoc->line,
                                                         funcStartLoc->column);
                        debugger::DebugHookHelper::enterFunctionHook(functionName, errorsLoc);
                    }
                    else
                    {
                        debugger::DebugHookHelper::enterFunctionHook(functionName, errors::SourceLocation());
                    }
                }
            }

            for (size_t i = 0; i < argCount; ++i)
            {
                context.stackManager->push(args[i]);
            }

            // Initialize remaining locals (beyond parameters) to null so the
            // debugger never sees uninitialized slots.
            for (size_t i = argCount; i < funcMetadata->localCount; ++i)
            {
                context.stackManager->push(std::monostate{});
            }

            context.instructionPointer = funcMetadata->startOffset - 1;  // -1 because the dispatch loop will increment
            return;
        }

        throw errors::RuntimeException("Function not found: " + functionName);
    }

    void FunctionExecutor::handleCallFast(const bytecode::BytecodeProgram::Instruction& instr)
    {
        size_t funcIndex = instr.inlineOperands[0];
        size_t argCount = instr.inlineOperands[1];

        const auto* funcMetadata = context.program->getFunctionByIndex(funcIndex);
        if (!funcMetadata) {
            throw errors::RuntimeException("CALL_FAST: invalid function index " + std::to_string(funcIndex));
        }

        size_t frameBase = context.stackManager->size() - argCount;

        // MYT-196: small-buffer-optimized args buffer.
        value::SmallArgsBuffer args(argCount);
        for (size_t i = argCount; i > 0; --i)
        {
            args[i - 1] = context.stackManager->pop();
        }

        if (!funcMetadata->allPrimitiveParams)
            convertLambdaArgumentsToInterfaces(args.span(), funcMetadata->parameterTypes);

        CallFrame frame;
        frame.returnAddress = context.instructionPointer;
        frame.frameBase = frameBase;
        frame.localBase = context.stackManager->size();
        // MYT-197: intern once. mangledName is already registered with the
        // program (see registerFunction), so this lookup is a hashmap hit
        // after the first call site — no string rebuilding per call.
        frame.functionName = context.program->internFrameName(
            funcMetadata->mangledName.empty() ? funcMetadata->name : funcMetadata->mangledName);
        frame.thisInstance = nullptr;
        frame.programIndex = context.callStack.empty() ? 0 : context.callStack.back().programIndex;

        context.pushCallFrame(std::move(frame));
        context.stats.functionCalls++;

        // Match the frame name (mangled when overloadable) so the profiler's
        // entry/exit pair balances against ControlFlowExecutor's RETURN, which
        // reports the frame's interned name on exit.
        vm::profiler::ProfilerHookHelper::onFunctionEntry(
            funcMetadata->mangledName.empty() ? funcMetadata->name : funcMetadata->mangledName);

        if (debugger::DebugHookHelper::isDebuggingEnabled())
        {
            auto sourceLoc = context.program->getSourceLocation(context.instructionPointer);
            if (sourceLoc)
            {
                errors::SourceLocation errorsLoc(sourceLoc->filename, sourceLoc->line, sourceLoc->column);
                debugger::DebugHookHelper::enterFunctionHook(funcMetadata->name, errorsLoc);
            }
            else
            {
                auto funcStartLoc = context.program->getSourceLocation(funcMetadata->startOffset);
                if (funcStartLoc)
                {
                    errors::SourceLocation errorsLoc(funcStartLoc->filename, funcStartLoc->line,
                                                     funcStartLoc->column);
                    debugger::DebugHookHelper::enterFunctionHook(funcMetadata->name, errorsLoc);
                }
                else
                {
                    debugger::DebugHookHelper::enterFunctionHook(funcMetadata->name, errors::SourceLocation());
                }
            }
        }

        for (size_t i = 0; i < argCount; ++i)
        {
            context.stackManager->push(args[i]);
        }

        for (size_t i = argCount; i < funcMetadata->localCount; ++i)
        {
            context.stackManager->push(std::monostate{});
        }

        context.instructionPointer = funcMetadata->startOffset - 1;
    }
}
