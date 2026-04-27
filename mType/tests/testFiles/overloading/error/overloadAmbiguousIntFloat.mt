// Test: Two overloads f(Int) and f(Float) — calling f(5) is ambiguous because
// the int literal can box to Int OR widen+box to Float. The compiler must
// reject with an ambiguity error rather than silently picking one.

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Float.mt";

function f(Int x): void {
    print("Int");
}

function f(Float x): void {
    print("Float");
}

function main(): void {
    f(5);
}

main();
