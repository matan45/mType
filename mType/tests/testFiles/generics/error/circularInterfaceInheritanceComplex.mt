// Test: Complex circular interface inheritance (A -> B -> C -> A)
// Should fail at parse time with clear error message

interface A extends B {
    function getValue(): int;
}

interface B extends C {
    function getData(): int;
}

interface C extends A {
    function getOther(): int;
}
