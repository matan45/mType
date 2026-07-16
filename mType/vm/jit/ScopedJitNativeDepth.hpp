#pragma once
#include "../runtime/VirtualMachine.hpp"

namespace vm::jit
{
    // RAII guard for VirtualMachine::jitNativeDepth. The counter defers GC
    // while generated code may hold boxed values in native frames that are
    // not published as VM roots, so a leaked increment defers collection
    // permanently — bracket every JIT entry with this guard instead of
    // manual ++/-- so unwinding cannot skip the decrement.
    class ScopedJitNativeDepth
    {
    public:
        explicit ScopedJitNativeDepth(vm::runtime::VirtualMachine& vm)
            : vm(vm)
        {
            vm.incrementJitNativeDepth();
        }

        ~ScopedJitNativeDepth() { vm.decrementJitNativeDepth(); }

        ScopedJitNativeDepth(const ScopedJitNativeDepth&) = delete;
        ScopedJitNativeDepth& operator=(const ScopedJitNativeDepth&) = delete;

    private:
        vm::runtime::VirtualMachine& vm;
    };
}
