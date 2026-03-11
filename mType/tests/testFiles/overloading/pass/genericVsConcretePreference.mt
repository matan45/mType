// Test: Concrete overloads preferred over generic, half-generic over full-generic
import * from "../../lib/primitives/Bool.mt";
import * from "../../lib/primitives/Int.mt";

function describe(int x, string y): string {
    return "concrete: int, string";
}

function <T> describe(T x, string y): string {
    return "half-generic: T, string";
}

function <T, U> describe(T x, U y): string {
    return "full-generic: T, U";
}

function main(): void {
    print("=== Generic vs Concrete Preference ===");

    // Concrete wins over both generics
    print(describe(42, "hello"));

    // Half-generic (T=Bool, string exact)
    print(describe<Bool>(true, "hello"));

    // Full-generic
    print(describe<Bool, Int>(true, 1));
}
main();
