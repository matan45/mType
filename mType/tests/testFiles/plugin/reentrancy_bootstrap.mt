// Bootstrap for PluginTestSuite reentrancy test. Provides an mType-side
// function the plugin trampoline can call back into via hostCallFunction.
// Loaded with parseAndRegisterClasses so top-level code is not executed.

function double(int n): int {
    return n * 2;
}
