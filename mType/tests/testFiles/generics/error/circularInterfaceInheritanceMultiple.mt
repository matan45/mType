// Test: Circular interface inheritance with multiple parents (C extends A, B; A extends C)
// Should fail at parse time with clear error message

interface A extends C {
    function getValue(): int;
}

interface B {
    function getData(): int;
}

interface C extends A, B {
    function getOther(): int;
}
