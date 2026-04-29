// Pinned behavior: f(Int) vs f(Float) called with the int literal 5.
// mType silently picks one overload rather than reporting ambiguity.
// This test documents the observed choice.

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
