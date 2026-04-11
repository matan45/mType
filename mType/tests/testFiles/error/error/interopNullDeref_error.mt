// MYT-46 — exercise the VM interop wrapper's location-decoration path.
// When the null deref fires from inside the wrapped invokeMethod path,
// the typed catch in VirtualMachine::executeFunction (and friends) walks
// the live callStack via InteropExceptionDecorator, attaches a real
// SourceLocation to the exception, and re-throws. The substring filter
// in ErrorTestSuite.cpp pins the test to the NullPointerException path.

class Probe {
    function trigger(): void {
        Probe other = null;
        other.trigger();
    }
}

Probe p = new Probe();
p.trigger();
