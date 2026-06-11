// Bootstrap for PluginTestSuite deep-reentrancy and object-probe tests.
// Loaded with parseAndRegisterClasses so top-level code is not executed.
// `bounce` re-enters the synthetic plugin native, building a
// plugin -> mType -> plugin -> ... chain. The native is registered by the
// test AFTER this file parses; CompileTimeValidator only skips call-target
// validation for names carrying the __native__ prefix (its marker for
// runtime-resolved plugin natives), hence the function name below.

function bounce(int d): int {
    return __native__pt_deep_recurse(d - 1) + 1;
}

class ProbeBox {
    public int x;

    public constructor() {
        x = 1;
    }

    public function getX(): int {
        return x;
    }
}
