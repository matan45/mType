// Test: Inheritance - child type passed to parent-typed parameter via single overload
class A {
    constructor() {}
}

class B extends A {
    constructor() {}
}

class C extends B {
    constructor() {}
}

class D extends C {
    constructor() {}
}

// Single overload accepting base type - all children should match via inheritance
function identifyAsA(A obj): string {
    return "matched A";
}

// Single overload accepting mid-level type
function identifyAsB(B obj): string {
    return "matched B";
}

// Two overloads with different unrelated param types - no ambiguity
function categorize(A obj, int x): string {
    return "A with int: " + x;
}

function categorize(A obj, string s): string {
    return "A with string: " + s;
}

function main(): void {
    print("=== Deep Inheritance Distance ===");

    D d = new D();
    C c = new C();
    B b = new B();
    A a = new A();

    // All should match via inheritance to A
    print(identifyAsA(a));   // exact
    print(identifyAsA(b));   // B -> A
    print(identifyAsA(c));   // C -> B -> A
    print(identifyAsA(d));   // D -> C -> B -> A

    // B and children should match via inheritance to B
    print(identifyAsB(b));   // exact
    print(identifyAsB(c));   // C -> B
    print(identifyAsB(d));   // D -> C -> B

    // Overloads distinguished by second param type, not inheritance
    print(categorize(d, 42));
    print(categorize(d, "hello"));
}
main();
