// MYT-376 canary: a genuinely runtime value (the result of an instance method
// call) is NOT a compile-time constant and must still be rejected. This guards
// the boundary — folding accepts compile-time constants only, never arbitrary
// runtime evaluation.

annotation Timeout {
    int ms;
}

class Clock {
    public function now(): int {
        return 42;
    }
}

@Timeout(ms = new Clock().now())
class Service { }
