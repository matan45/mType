// Test: Complex circular class inheritance (A -> B -> C -> A)
// Should fail at parse time with clear error message

class A extends B {
    int value;
}

class B extends C {
    int data;
}

class C extends A {
    int other;
}
