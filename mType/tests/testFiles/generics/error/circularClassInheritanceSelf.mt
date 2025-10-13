// Test: Self-inheritance (A extends A)
// Should fail at parse time with clear error message

class A extends A {
    int value;
}
