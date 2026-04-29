// ERROR: Direct circular interface inheritance - A extends B, B extends A.
// Must be rejected by the compiler.

interface A extends B {
    function methodA(): void;
}

interface B extends A {
    function methodB(): void;
}
