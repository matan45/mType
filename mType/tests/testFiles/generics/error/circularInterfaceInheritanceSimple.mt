// Test: Simple circular interface inheritance (A -> B -> A)
// Should fail at parse time with clear error message

interface A extends B {
    function getValue(): int;
}

interface B extends A {
    function getData(): int;
}
