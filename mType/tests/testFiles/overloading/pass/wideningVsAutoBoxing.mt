// Test: Numeric widening (priority 1) should be preferred over auto-boxing (priority 2)
import * from "../../lib/primitives/Int.mt";

function convert(float x): string {
    return "widened to float";
}

function convert(Int x): string {
    return "auto-boxed to Int";
}

function main(): void {
    print("=== Widening vs AutoBoxing ===");

    // int -> float (NUMERIC_WIDENING=1) should beat int -> Int (AUTO_BOXING=2)
    print(convert(42));

    // float is exact match
    print(convert(3.14));
}
main();
