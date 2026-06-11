// Bootstrap for PluginTestSuite deep-reentrancy and object-probe tests.
// Loaded with parseAndRegisterClasses so top-level code is not executed.
// `bounce` re-enters the synthetic plugin native, building a
// plugin -> mType -> plugin -> ... chain; the native is registered by the
// test AFTER this file parses, which works because unknown call targets are
// resolved at runtime (same late binding that lets scripts call natives
// loaded via __plugin_load mid-execution).

function bounce(int d): int {
    return __pt_deep_recurse(d - 1) + 1;
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
