// Test: When both a concrete `f(int)` and a generic `f<T>(T)` overload match,
// the concrete one is preferred. The generic one is selected only when the
// concrete signature does not apply.

import * from "../../lib/primitives/String.mt";

function <T> f(T x): string {
    return "generic";
}

function f(int x): string {
    return "concrete int";
}

function main(): void {
    // Concrete int overload wins for int literal.
    print(f(5));

    // No concrete string overload exists, so the generic is selected.
    print(f<String>(new String("hello")));
}

main();
