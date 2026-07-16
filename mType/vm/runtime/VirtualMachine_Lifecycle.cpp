#include "VirtualMachine.hpp"
#include <cstddef>
#include <sstream>
#include "../../errors/RuntimeException.hpp"
#include "../../errors/UserException.hpp"
#include "../../gc/GC.hpp"
#include "../../runtime/EventLoop.hpp"
#include "../../value/ObjectInstance.hpp"
#include "../jit/JitCodeCache.hpp"
#include "../jit/JitProfiler.hpp"
#include "../jit/OSRManager.hpp"
#include "../jit/ic/InlineCacheTable.hpp"

namespace
{
    void appendStackObjectRoots(
        std::vector<void*>& roots,
        runtimeTypes::klass::ObjectInstance* instance)
    {
        if (!instance) return;
        // Stack-promoted instances are intentionally not tracked. Their heap
        // fields, rather than the borrowed instance address, are GC roots.
        instance->visitReferences([&roots](void* reference) {
            if (reference) roots.push_back(reference);
        });
    }
}

namespace vm::runtime
{
    void VirtualMachine::runStaticInitializers(const bytecode::BytecodeProgram& bytecodeProgram)
    {
        // Idempotency check first — return immediately for an already-
        // initialized program without touching `program` or executionCtx.
        if (staticInitializedPrograms.count(&bytecodeProgram) > 0)
        {
            return;
        }

        bool isLoadedProgram = false;
        for (const auto* loadedProgram : loadedPrograms)
        {
            if (loadedProgram == &bytecodeProgram)
            {
                isLoadedProgram = true;
                break;
            }
        }

        if (!isLoadedProgram && program != &bytecodeProgram)
        {
            setProgram(&bytecodeProgram);
        }

        const auto& initializerNames = bytecodeProgram.getStaticInitializerFunctions();
        if (initializerNames.empty())
        {
            staticInitializedPrograms.insert(&bytecodeProgram);
            return;
        }

        const bytecode::BytecodeProgram* savedProgram = program;
        size_t savedIP = instructionPointer;
        std::vector<CallFrame> savedCallStack = callStack;
        size_t savedCurrentFinallyOffset = currentFinallyOffset;
        // Capture the operand-stack high-water mark BEFORE the try so a throw
        // partway through an initializer can drain leftover pushes back to
        // the caller's view. Without this, an exception escaping mid-init
        // left the operand stack with phantom values.
        size_t savedStackSize = stackManager->size();

        if (!program)
        {
            setProgram(&bytecodeProgram);
        }
        else
        {
            program = &bytecodeProgram;
            if (executionCtx)
            {
                executionCtx->program = &bytecodeProgram;
            }
        }

        size_t programIndex = 0;
        for (size_t i = 0; i < loadedPrograms.size(); ++i)
        {
            if (loadedPrograms[i] == &bytecodeProgram)
            {
                programIndex = i;
                break;
            }
        }

        auto restoreState = [&]() {
            while (stackManager->size() > savedStackSize)
            {
                stackManager->pop();
            }
            instructionPointer = savedIP;
            callStack = savedCallStack;
            currentFinallyOffset = savedCurrentFinallyOffset;
            program = savedProgram;
            if (executionCtx)
            {
                executionCtx->program = savedProgram;
            }
        };

        try
        {
            for (const auto& initializerName : initializerNames)
            {
                // MYT-325: skip duplicates when the same class lives in
                // multiple loaded programs (e.g. embedded main bytecode plus a
                // sidecar .mtcLib that also defines the class). The synthetic
                // name "<ClassName>::<static_init>$static" is identical across
                // every program carrying that class, so insert-and-check by
                // name dedups regardless of program identity.
                if (!executedStaticInitializers.insert(initializerName).second)
                {
                    continue;
                }

                auto* funcMetadata = bytecodeProgram.getFunction(initializerName);
                if (!funcMetadata)
                {
                    throw errors::RuntimeException(
                        "Static initializer '" + initializerName + "' has no bytecode");
                }

                size_t frameBase = stackManager->size();

                CallFrame frame;
                frame.returnAddress = bytecodeProgram.getInstructionCount();
                frame.frameBase = frameBase;
                frame.localBase = frameBase;
                frame.functionName = bytecodeProgram.internFrameName(initializerName);
                frame.thisInstance = nullptr;
                frame.programIndex = programIndex;

                size_t colonPos = initializerName.find("::");
                if (colonPos != std::string::npos)
                {
                    frame.definingClassName = initializerName.substr(0, colonPos);
                }

                pushCallFrame(std::move(frame));
                stats.functionCalls++;
                instructionPointer = funcMetadata->startOffset;

                interpretLoop();

                while (stackManager->size() > frameBase)
                {
                    stackManager->pop();
                }
            }

            staticInitializedPrograms.insert(&bytecodeProgram);
            restoreState();
        }
        catch (...)
        {
            restoreState();
            throw;
        }
    }

    void VirtualMachine::runStaticInitializersForLoadedPrograms()
    {
        if (loadedPrograms.empty())
        {
            if (program)
            {
                runStaticInitializers(*program);
            }
            return;
        }

        for (size_t i = 1; i < loadedPrograms.size(); ++i)
        {
            if (loadedPrograms[i])
            {
                runStaticInitializers(*loadedPrograms[i]);
            }
        }

        if (loadedPrograms[0])
        {
            runStaticInitializers(*loadedPrograms[0]);
        }
    }

    void VirtualMachine::pushCallFrame(CallFrame frame)
    {
        if (callStack.size() >= maxCallStackSize)
        {
            // MYT-228: clear any stale pending bindings so a future call
            // can't pick them up after the exception is caught. Mirrors
            // the same guard in ExecutionContext::pushCallFrame.
            if (executionCtx)
            {
                executionCtx->pendingTypeArgs.reset();
            }

            std::ostringstream oss;
            oss << "Stack overflow: Maximum call stack depth of "
                << maxCallStackSize << " exceeded.\n";
            oss << "This may indicate infinite recursion.\n";
            oss << "Call stack trace (most recent call first):\n";

            auto frameNameOf = [&](const CallFrame& f) -> const std::string& {
                const bytecode::BytecodeProgram* p =
                    (f.programIndex < loadedPrograms.size())
                        ? loadedPrograms[f.programIndex]
                        : program;
                return p->getFrameName(f.functionName);
            };
            size_t startIdx = callStack.size() > 10 ? callStack.size() - 10 : 0;
            for (size_t i = startIdx; i < callStack.size(); ++i)
            {
                oss << "  [" << i << "] " << frameNameOf(callStack[i]);
                if (!callStack[i].definingClassName.empty())
                {
                    oss << " (in class " << callStack[i].definingClassName << ")";
                }
                oss << "\n";
            }

            oss << "  [" << callStack.size() << "] " << frameNameOf(frame);
            if (!frame.definingClassName.empty())
            {
                oss << " (in class " << frame.definingClassName << ")";
            }
            oss << " <- stack overflow here\n";

            throw errors::RuntimeException(oss.str());
        }

        // MYT-228: same consume-and-clear convention as
        // ExecutionContext::pushCallFrame so this VM-level entry stays in sync.
        // Hot path: pendingTypeArgs is null, single branch.
        if (executionCtx && executionCtx->pendingTypeArgs)
        {
            frame.typeArgBindings.adopt(executionCtx->pendingTypeArgs.releasePtr());
        }

        callStack.push_back(std::move(frame));
    }

    void VirtualMachine::popCallStack()
    {
        if (!callStack.empty())
        {
            // MYT-208: release stack-promoted allocations before pop.
            callStack.back().releaseStackObjects();
            callStack.pop_back();
        }
    }

    std::vector<std::string> VirtualMachine::getStackTrace() const
    {
        std::vector<std::string> trace;
        for (const auto& frame : callStack)
        {
            const bytecode::BytecodeProgram* p =
                (frame.programIndex < loadedPrograms.size())
                    ? loadedPrograms[frame.programIndex]
                    : program;
            std::ostringstream oss;
            oss << p->getFrameName(frame.functionName)
                << " at offset " << frame.returnAddress;
            trace.push_back(oss.str());
        }
        return trace;
    }

    void VirtualMachine::reset()
    {
        stackManager->clear();
        // MYT-208: release every frame's stack-promoted allocations before
        // clearing callStack — otherwise the pool leaks recyclable slots
        // every time reset() is invoked (e.g. uncaught-exception cleanup).
        for (auto& frame : callStack)
        {
            frame.releaseStackObjects();
        }
        callStack.clear();
        instructionPointer = 0;
        stats = ExecutionStats{};

        // MYT-A4: drop all JIT-side state that points at script-defined
        // ClassDefinitions. Rebuild and BytecodeExecutionStrategy program
        // replacement both reset before freeing the old program; rebuild
        // additionally clears script definitions. IC entries (InlineCacheTable),
        // cached compiled stubs (JitCodeCache), and feedback counters
        // (JitProfiler) all hold raw pointers into that lifetime. Without
        // this clear, the next compile-and-run with the same VM dispatches
        // through stale stubs and dereferences freed CDs (use-after-free).
        // OSR owns non-owning JitFunction pointers into JitCodeCache. Clear
        // those first so a rebuilt program can never observe released code.
        if (osrManager) osrManager->reset();
        if (jitCodeCache) jitCodeCache->clear();
        if (jitProfiler)  jitProfiler->reset();
        if (inlineCacheTable) inlineCacheTable->clear();

        // MYT-370 (rebuild regression): these sets dedup static initializers
        // within a single build (MYT-325: same class in main bytecode + a
        // sidecar .mtcLib). They hold stale BytecodeProgram* and synthetic
        // "<Class>::<static_init>$static" names from the program being torn
        // down. Rebuild/program-replacement callers free that program only
        // after reset; rebuild then recompiles definitions whose static int fields
        // default to 0. Without clearing here, runStaticInitializers() skips
        // the re-run (name already seen) and constants like Mouse::RIGHT
        // silently read 0.
        staticInitializedPrograms.clear();
        executedStaticInitializers.clear();
    }

    std::vector<void*> VirtualMachine::collectGCRoots() const
    {
        // Generated frames keep boxed operands and boxed locals in native
        // stack storage that is not currently described by this root API.
        // If an explicit/native callback forces collection while such a frame
        // is live, conservatively root the tracked heap. Automatic polls are
        // deferred, so this is only the fail-safe path.
        if (jitNativeDepth != 0)
        {
            return gc::GCTracker::getInstance().getAllTrackedPointers();
        }

        std::vector<void*> roots;

        const auto& stack = stackManager->getStack();
        for (const auto& val : stack)
        {
            void* ptr = gc::extractPointer(val);
            if (ptr)
            {
                roots.push_back(ptr);
            }
        }

        for (const auto& frame : callStack)
        {
            if (frame.thisInstance)
            {
                roots.push_back(frame.thisInstance.get());
            }

            // MYT-208: stack-promoted `this` (NEW_STACK ctor) and frame-owned
            // raw allocations. These are not GC-registered, but their fields
            // may hold heap references that the cycle detector must keep alive
            // while the owning frame is live. Inline array iteration over
            // [0, stackObjectsCount) — entries beyond count are uninitialised.
            if (frame.thisInstanceRaw)
            {
                appendStackObjectRoots(roots, frame.thisInstanceRaw);
            }
            for (size_t i = 0; i < frame.stackObjectsCount; ++i)
            {
                if (frame.stackObjects[i])
                {
                    appendStackObjectRoots(roots, frame.stackObjects[i]);
                }
            }

            if (frame.sharedFrame)
            {
                for (const auto& local : frame.sharedFrame->locals)
                {
                    void* ptr = gc::extractPointer(local);
                    if (ptr)
                    {
                        roots.push_back(ptr);
                    }
                }
            }

            if (frame.originatingLambda)
            {
                roots.push_back(frame.originatingLambda.get());
            }
        }

        // Registry-owned Values are process-visible roots even when no copy is
        // currently on the operand stack (globals and static fields).
        if (environment)
        {
            if (auto variables = environment->getVariableManager())
            {
                for (const auto& name : variables->getAllVariableNames())
                {
                    if (auto variable = variables->findVariable(name))
                    {
                        if (void* ptr = gc::extractPointer(variable->getValue()))
                            roots.push_back(ptr);
                    }
                }
            }
            if (auto classes = environment->getClassRegistry())
            {
                for (const auto& name : classes->getAllItemNames())
                {
                    auto classDefinition = classes->findClass(name);
                    if (!classDefinition) continue;
                    for (const auto& [_, field] : classDefinition->getStaticFields())
                    {
                        if (field)
                        {
                            if (void* ptr = gc::extractPointer(field->getValue()))
                                roots.push_back(ptr);
                        }
                    }
                }
            }
        }

        // Async suspension keeps a second stack/frame graph outside the live
        // StackManager. These Values remain roots until restore/cancellation.
        if (savedState)
        {
            for (const auto& val : savedState->stack)
            {
                if (void* ptr = gc::extractPointer(val)) roots.push_back(ptr);
            }
            for (const auto& frame : savedState->callStack)
            {
                if (frame.thisInstance) roots.push_back(frame.thisInstance.get());
                if (frame.thisInstanceRaw)
                    appendStackObjectRoots(roots, frame.thisInstanceRaw);
                for (size_t i = 0; i < frame.stackObjectsCount; ++i)
                {
                    if (frame.stackObjects[i])
                        appendStackObjectRoots(roots, frame.stackObjects[i]);
                }
                if (frame.sharedFrame)
                {
                    for (const auto& local : frame.sharedFrame->locals)
                    {
                        if (void* ptr = gc::extractPointer(local)) roots.push_back(ptr);
                    }
                }
                if (frame.originatingLambda) roots.push_back(frame.originatingLambda.get());
            }
        }

        if (pendingAwaitRejection)
        {
            if (void* ptr = gc::extractPointer(pendingAwaitRejection->exceptionValue))
                roots.push_back(ptr);
        }
        if (interopPendingRejection)
        {
            if (void* ptr = gc::extractPointer(interopPendingRejection->exceptionValue))
                roots.push_back(ptr);
        }
        if (interopAwaitedPromise) roots.push_back(interopAwaitedPromise.get());
        if (pendingException)
        {
            if (void* ptr = gc::extractPointer(pendingException->getExceptionValue()))
                roots.push_back(ptr);
        }
        if (eventLoop)
        {
            eventLoop->visitGCRoots(
                [&roots](const value::Value& val) {
                    if (void* ptr = gc::extractPointer(val)) roots.push_back(ptr);
                },
                [&roots](value::PromiseValue* promise) {
                    if (promise) roots.push_back(promise);
                });
        }

        return roots;
    }
}
