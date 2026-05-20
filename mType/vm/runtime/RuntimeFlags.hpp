#pragma once

namespace vm::runtime
{
    // Runtime-toggleable behavior. Defaults match optimal behavior; flipping
    // a flag here forces the interpreter and JIT to fall back to the pre-
    // optimization codepath without rebuilding.
    struct RuntimeFlags
    {
        // When true, STACK_SCOPE_ENTER / STACK_SCOPE_LEAVE opcodes are treated
        // as no-ops by both the interpreter and JIT helpers. Stack-promoted
        // NEW_STACK allocations then live until frame teardown (the pre-
        // per-scope-release behavior — useful for A/B comparison without
        // recompiling).
        static inline bool disableStackScopeRelease = false;
    };
}
