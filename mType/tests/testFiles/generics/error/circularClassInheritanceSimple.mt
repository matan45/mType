// Test: Simple circular class inheritance (A -> B -> A)
// Should fail at parse time with clear error message

class A extends B {
    int value;
}

class B extends A {
    int data;
}
